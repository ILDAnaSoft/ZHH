from __future__ import annotations

from abc import ABC
from typing import Any, TYPE_CHECKING
import numpy as np

if TYPE_CHECKING:
    import torch
    from ..TTreeInterface import FinalStateCounts

try:
    import torch
    import torch.nn as nn
    import torch.nn.functional as F
    _TORCH_AVAILABLE = True
except ImportError:
    _TORCH_AVAILABLE = False


def _require_torch():
    if not _TORCH_AVAILABLE:
        raise ImportError(
            'ModelDistributionAsFlowAction requires the optional "torch" dependency. '
            'Install it, e.g. via `pip install torch`.'
        )


# ---------------------------------------------------------------------------
# Feature transforms
#
# Each property is transformed to an (approximately) unbounded real value before
# being fed into the flow, and back-transformed (backward()) when interpreting
# generated samples. See parse_properties() for the corresponding YAML format.
# ---------------------------------------------------------------------------

class LogitNamedProperty(ABC):
    def __init__(self, name: str, lower: float | int | None, upper: float | int | None):
        self.name = name
        self.lower = lower
        self.upper = upper

    def forward(self, x: np.ndarray) -> np.ndarray:
        return x

    def backward(self, y: np.ndarray) -> np.ndarray:
        return y

    def __repr__(self):
        return f'{self.__class__.__name__}({self.name}, lower={self.lower}, upper={self.upper})'


class LogitBoundedProperty(LogitNamedProperty):
    def __init__(self, name: str, lower: float | int, upper: float | int, eps: float = 1e-8):
        """Transforms any variable by bounding the output to [lower, upper], rescaling
        to (0, 1), and subsequently using the logit function.
        """
        assert lower is not None and upper is not None, 'Lower and upper bounds must be specified.'

        self.eps = eps
        super().__init__(name, lower, upper)

    def forward(self, x: np.ndarray) -> np.ndarray:
        # Bound to [lower, upper], then rescale to (0, 1) -- the logit below is only
        # meaningful on that domain. Without this rescale, y/(1-y) goes negative for
        # any value above 1 (i.e. for any bound other than upper=1), and log() of that
        # produces NaN.
        y = np.clip(x, self.lower, self.upper)
        y = (y - self.lower) / (self.upper - self.lower)
        y = np.clip(y, self.eps, 1 - self.eps)
        y = y / (1 - y)
        y = np.log(y)

        return y

    def backward(self, y: np.ndarray) -> np.ndarray:
        x = np.exp(y)
        x = x / (1 + x)  # sigmoid, in (0, 1)

        # Undo the rescale-to-(0,1) from forward.
        x = x * (self.upper - self.lower) + self.lower

        return x


class LogitIntProperty(LogitBoundedProperty):
    def __init__(self, name: str, lower: int, upper: int, **kwargs):
        """Transforms any variable by rounding, converting to int
        and bounding the output to [lower, upper].
        """
        super().__init__(name, lower, upper, **kwargs)

    def backward(self, y: np.ndarray) -> np.ndarray:
        x = super().backward(y)

        return np.round(np.clip(x, self.lower, self.upper)).astype(np.int32)


class LogitProbabilityProperty(LogitBoundedProperty):
    def __init__(self, name: str, eps: float = 1e-8):
        """Transforms a probability variable within [0, 1] using the logit function
        and bounds the output to [lower, upper] to avoid infinities.
        """
        super().__init__(name, 0, 1, eps=eps)


_PROPERTY_TYPES: dict[str, type] = {
    'identity': LogitNamedProperty,
    'bounded': LogitBoundedProperty,
    'int': LogitIntProperty,
    'probability': LogitProbabilityProperty,
}


def parse_properties(properties: list[dict[str, Any]]) -> list[LogitNamedProperty]:
    """Parses a list of dicts (as loaded from YAML) into LogitNamedProperty instances
    used to transform/untransform each feature dimension trained by
    ModelDistributionAsFlowAction.

    Format of a single item:
        name: <str>                                # required; column name in the DataStore/pickle dump
        type: identity|bounded|int|probability      # optional, default: identity
        lower: <float|int>                          # required for type: bounded, int
        upper: <float|int>                          # required for type: bounded, int
        eps: <float>                                # optional (default: 1e-8); bounded/int/probability only

    Example:
        properties:
          - name: jet1_m
            type: bounded
            lower: 0
            upper: 650
          - name: npfos
            type: int
            lower: 0
            upper: 250
          - name: bmax1
            type: probability
          - name: costhrust
            type: bounded
            lower: -1
            upper: 1
    """
    parsed: list[LogitNamedProperty] = []

    for spec in properties:
        spec = dict(spec)
        name = spec.pop('name')
        type_ = spec.pop('type', 'identity')

        if type_ not in _PROPERTY_TYPES:
            raise ValueError(
                f'Unknown property type <{type_}> for property <{name}>. '
                f'Must be one of {list(_PROPERTY_TYPES.keys())}.'
            )

        cls = _PROPERTY_TYPES[type_]

        if cls is LogitNamedProperty:
            parsed.append(cls(name, spec.pop('lower', None), spec.pop('upper', None)))
        elif cls is LogitProbabilityProperty:
            parsed.append(cls(name, **spec))
        else:
            lower = spec.pop('lower')
            upper = spec.pop('upper')
            parsed.append(cls(name, lower, upper, **spec))

    return parsed


# ---------------------------------------------------------------------------
# Conditioning vector / data extraction
#
# Both the DataStore-based path (ModelDistributionAsFlowAction._extractFromStore)
# and the pickle-dump-based path (ModelDistributionAsFlowAction.from_pickle_files)
# ultimately produce the same triple (data, fsc_coded, conditioning_vector) consumed
# by the training/plotting code below.
# ---------------------------------------------------------------------------

def extract_conditioning_vector(fsc: 'FinalStateCounts', process: np.ndarray, pol_code: np.ndarray) -> np.ndarray:
    """Builds a per-event conditioning vector of [process_index, pol_code, <12 final
    state counts>] used to condition the flow on the hard process and final state
    topology of an event.
    """
    conditioning_vector = np.ndarray((len(fsc), 12 + 2), dtype=np.int32)

    process_unq = np.unique(process)
    for i, proc in enumerate(process_unq):
        conditioning_vector[process == proc, 0] = i

    conditioning_vector[:, 1] = pol_code

    for i, key in enumerate(['n_d', 'n_u', 'n_s', 'n_c', 'n_b',
                             'n_e', 'n_mu', 'n_tau',
                             'n_ve', 'n_vmu', 'n_vtau',
                             'n_b_from_higgs']):
        conditioning_vector[:, 2 + i] = getattr(fsc, key)

    return conditioning_vector


def extract_data(store, properties: list[LogitNamedProperty]) -> dict[str, np.ndarray]:
    """Extracts and forward-transforms feature data for the given properties from a
    DataStore (or any object supporting __getitem__ by column name).
    """
    data: dict[str, np.ndarray] = {}

    for prop in properties:
        data[prop.name] = prop.forward(np.asarray(store[prop.name], dtype=np.float64))

    return data


# ---------------------------------------------------------------------------
# Conditional Flow Matching (CFM) model
#
# Rather than an invertible transform trained by exact maximum likelihood (a
# normalizing flow), a velocity field v_theta(x_t, t) is trained with a plain
# regression objective along a fixed noise->data interpolation path. This is
# simulation-free at training time (no ODE solve / log-det Jacobian), at the cost
# of a numerical ODE integration when sampling (see sample_cfm).
# ---------------------------------------------------------------------------

if _TORCH_AVAILABLE:
    class VelocityField(nn.Module):
        """Predicts v_theta(x_t, t | context) ~ x1 - x0 along the linear noise->data path."""

        def __init__(self, features: int, time_dim: int, context_dim: int, hidden: int):
            super().__init__()
            self.time_dim = time_dim
            self.net = nn.Sequential(
                nn.Linear(features + time_dim + context_dim, hidden), nn.SiLU(),
                nn.Linear(hidden, hidden), nn.SiLU(),
                nn.Linear(hidden, hidden), nn.SiLU(),
                nn.Linear(hidden, hidden), nn.SiLU(),
                nn.Linear(hidden, hidden), nn.SiLU(),
                nn.Linear(hidden, hidden), nn.SiLU(),
                nn.Linear(hidden, features),
            )

        def forward(self, x, t, ctx=None):
            t_emb = sinusoidal_embedding(t, self.time_dim)
            parts = [x, t_emb] if ctx is None else [x, t_emb, ctx]
            return self.net(torch.cat(parts, dim=-1))


def sinusoidal_embedding(t: 'torch.Tensor', dim: int) -> 'torch.Tensor':
    import math

    half = dim // 2
    freqs = torch.exp(-math.log(10_000) * torch.arange(half, device=t.device) / half)
    args = t[:, None] * freqs[None, :]
    return torch.cat([torch.sin(args), torch.cos(args)], dim=-1)


def _build_modules(n_features: int, time_dim: int, hidden: int, context_dim: int,
                    embed_dim: int, conditioned: bool, device: 'torch.device'):
    _require_torch()

    velocity_field = VelocityField(
        features=n_features, time_dim=time_dim,
        context_dim=embed_dim if conditioned else 0, hidden=hidden,
    ).to(device)

    cond_embedding = None
    if conditioned:
        cond_embedding = nn.Sequential(
            nn.Linear(context_dim, 64), nn.ReLU(), nn.Linear(64, embed_dim),
        ).to(device)

    return velocity_field, cond_embedding


def _state_dict_to_cpu(module) -> dict:
    return {k: v.detach().clone().cpu() for k, v in module.state_dict().items()}


def train_cfm(x_norm: np.ndarray, cond_norm: np.ndarray | None, *,
              time_dim: int = 32, hidden: int = 1024, embed_dim: int = 32,
              epochs: int = 64, lr: float = 9e-5, batch_size: int = 16384,
              max_norm: float = 100.0, device: str = 'cpu', verbose: bool = True) -> dict[str, Any]:
    """Trains a Conditional Flow Matching velocity field on x_norm (optionally
    conditioned on cond_norm) and returns a dict describing the best (lowest mean
    epoch loss) model, suitable for build_flow_model() and for pickling to disk.
    """
    _require_torch()
    from torch.utils.data import TensorDataset, DataLoader

    torch_device = torch.device(device)
    n_features = x_norm.shape[1]
    conditioned = cond_norm is not None
    context_dim = cond_norm.shape[1] if conditioned else 0

    velocity_field, cond_embedding = _build_modules(
        n_features, time_dim, hidden, context_dim, embed_dim, conditioned, torch_device)

    x_tensor = torch.tensor(x_norm, dtype=torch.float32, device=torch_device)
    dataset = TensorDataset(x_tensor, torch.tensor(cond_norm, dtype=torch.float32, device=torch_device)) \
        if conditioned else TensorDataset(x_tensor)

    n_events = len(dataset)
    batch_size = min(batch_size, n_events)
    loader = DataLoader(dataset, batch_size=batch_size, shuffle=True, drop_last=n_events > batch_size)

    params = list(velocity_field.parameters()) + (list(cond_embedding.parameters()) if conditioned else [])
    optimizer = torch.optim.Adam(params, lr=lr)

    best_loss = float('inf')
    best_state: dict[str, Any] | None = None
    best_epoch = -1

    for epoch in range(epochs):
        epoch_loss = 0.
        n_batches = 0

        for batch in loader:
            if conditioned:
                x1, cond_batch = batch
                ctx = cond_embedding(cond_batch)
            else:
                x1 = batch[0]
                ctx = None

            # Linear (rectified-flow) interpolation between noise x0 and data x1:
            # x_t = (1-t) x0 + t x1 has constant velocity x1 - x0 along the whole
            # path, which is the regression target.
            x0 = torch.randn_like(x1)
            t = torch.rand(x1.shape[0], device=torch_device)
            xt = (1 - t[:, None]) * x0 + t[:, None] * x1
            target = x1 - x0

            v_pred = velocity_field(xt, t, ctx)
            loss = F.mse_loss(v_pred, target)

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(params, max_norm=max_norm)
            optimizer.step()

            epoch_loss += loss.item()
            n_batches += 1

        mean_loss = epoch_loss / max(n_batches, 1)

        if verbose:
            print(f'[ModelDistributionAsFlowAction] epoch {epoch + 1}/{epochs} loss={mean_loss:.4f} '
                  f'(best={best_loss:.4f})')

        if mean_loss < best_loss:
            best_loss = mean_loss
            best_epoch = epoch
            best_state = {
                'velocity_field_state': _state_dict_to_cpu(velocity_field),
                'embedding_state': _state_dict_to_cpu(cond_embedding) if conditioned else None,
            }

    assert best_state is not None

    return {
        **best_state,
        'loss': best_loss,
        'epoch': best_epoch,
        'n_features': n_features,
        'context_dim': context_dim,
        'embed_dim': embed_dim,
        'time_dim': time_dim,
        'hidden': hidden,
        'conditioned': conditioned,
    }


def build_flow_model(state: dict[str, Any], device: str = 'cpu'):
    """Reconstructs (velocity_field, cond_embedding) modules in eval mode from a
    dict as returned by train_cfm() (or loaded from a saved model_file).
    """
    _require_torch()

    torch_device = torch.device(device)
    velocity_field, cond_embedding = _build_modules(
        state['n_features'], state['time_dim'], state['hidden'], state['context_dim'],
        state['embed_dim'], state['conditioned'], torch_device)

    velocity_field.load_state_dict(state['velocity_field_state'])
    velocity_field.eval()

    if cond_embedding is not None:
        cond_embedding.load_state_dict(state['embedding_state'])
        cond_embedding.eval()

    return velocity_field, cond_embedding


_no_grad = torch.no_grad() if _TORCH_AVAILABLE else (lambda fn: fn)


@_no_grad
def sample_cfm(velocity_field, n_samples: int, n_features: int, ctx=None, steps: int = 50,
               device: str = 'cpu') -> 'torch.Tensor':
    """Integrate dx/dt = v_theta(x, t | ctx) from t=0 (noise) to t=1 (data) using a
    fixed-step RK4 solver; no external ODE-solver dependency needed.
    """
    x = torch.randn(n_samples, n_features, device=device)
    dt = 1.0 / steps

    for step in range(steps):
        t0 = torch.full((n_samples,), step * dt, device=device)

        k1 = velocity_field(x, t0, ctx)
        k2 = velocity_field(x + dt / 2 * k1, t0 + dt / 2, ctx)
        k3 = velocity_field(x + dt / 2 * k2, t0 + dt / 2, ctx)
        k4 = velocity_field(x + dt * k3, t0 + dt, ctx)
        x = x + (dt / 6) * (k1 + 2 * k2 + 2 * k3 + k4)

    return x
