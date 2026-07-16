from __future__ import annotations

import os
import os.path as osp
import pickle
from typing import Any, Sequence, TYPE_CHECKING

import numpy as np

from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from .flow_tools import (
    LogitNamedProperty, parse_properties, extract_data, extract_conditioning_vector,
    train_cfm, build_flow_model, sample_cfm, _require_torch
)

if TYPE_CHECKING:
    from ..DataSource import DataSource


class ModelDistributionAsFlowAction(FileBasedProcessorAction):
    # training can take a long time; abort cleanly (reset()) rather than leave a
    # half-written model_file/plots_file behind if interrupted
    interruptible = False

    def __init__(self, cp: CutflowProcessor | None, steer: dict, properties: list[dict],
                 model_file: str, plots_file: str,
                 source: str | int | None = None,
                 pickle_files: str | Sequence[str] | None = None,
                 conditioned: bool = True,
                 epochs: int = 64, lr: float = 9e-5, batch_size: int = 16384,
                 hidden: int = 1024, embed_dim: int = 32, time_dim: int = 32,
                 max_norm: float = 100.0, device: str | None = None,
                 ode_steps: int = 50, n_control_samples: int = 100_000,
                 control_plot_class: int | None = None,
                 nbins: int = 64, yscale: str = 'log',
                 seed: int | None = None, **kwargs):
        """Trains a Conditional Flow Matching (CFM) model to reproduce the joint
        distribution of a list of `properties` (see flow_tools.parse_properties() for
        the YAML format), conditioned on the hard process/final-state topology of
        each event. Writes the best (lowest training loss) model to `model_file` and
        per-feature control plots (source vs. learned distribution and their
        residual) to `plots_file`.

        Data can be sourced either from a DataStore registered with the supplied
        CutflowProcessor `cp` (the default; select which one via `source`), or --
        bypassing `cp` entirely -- from one or multiple pickle dumps as written by
        dumpStoreData(). See the alternate constructor from_pickle_files() for the
        latter, e.g. to move training onto a machine without access to the
        underlying ROOT/HDF5 data (a GPU node).

        Args:
            cp: CutflowProcessor holding the DataSource to train on. May be None
                only when `pickle_files` is used instead.
            steer: cutflow steering dict.
            properties (list[dict]): feature dimensions to train on, see
                flow_tools.parse_properties().
            model_file (str): output file the best model is written to (torch format).
            plots_file (str): output PDF file with control plots.
            source (str|int|None): name or index of the DataSource (within
                cp.getSources()) to train on. Ignored if `pickle_files` is given.
                If None and cp has exactly one registered source, that source is used.
            pickle_files (str|Sequence[str]|None): if given, ignore `cp`/`source` and
                load pre-extracted dumps instead, see from_pickle_files().
            conditioned (bool): whether to condition the flow on the process/final
                state topology conditioning vector. Defaults to True.
            epochs, lr, batch_size, hidden, embed_dim, time_dim, max_norm: CFM
                training hyperparameters.
            device (str|None): torch device, e.g. 'cuda' or 'cpu'. Autodetected if None.
            ode_steps (int): number of RK4 integration steps used for sampling.
            n_control_samples (int): number of samples drawn for the control plots.
            control_plot_class (int|None): fsc_coded value to condition sampling on
                for the control plots. If None, the most frequent value in the dataset
                is used. Ignored if conditioned=False.
            nbins, yscale: histogram plotting options for the control plots.
            seed (int|None): if given, seeds numpy/torch RNGs before training.
        """
        super().__init__(cp, steer)

        self._properties_raw = properties
        self._properties = parse_properties(properties)
        self._model_file = model_file
        self._plots_file = plots_file

        self._source = source
        self._pickle_files: list[str] | None = (
            [pickle_files] if isinstance(pickle_files, str)
            else list(pickle_files) if pickle_files is not None
            else None
        )

        if self._pickle_files is None:
            if cp is None:
                raise Exception('ModelDistributionAsFlowAction: `cp` may only be None when `pickle_files` is given.')

            if source is None and len(cp.getSources()) != 1:
                raise Exception('ModelDistributionAsFlowAction: `source` must be given explicitly unless '
                                 'the CutflowProcessor has exactly one registered DataSource.')

        self._conditioned = conditioned
        self._epochs = epochs
        self._lr = lr
        self._batch_size = batch_size
        self._hidden = hidden
        self._embed_dim = embed_dim
        self._time_dim = time_dim
        self._max_norm = max_norm
        self._device = device
        self._ode_steps = ode_steps
        self._n_control_samples = n_control_samples
        self._control_plot_class = control_plot_class
        self._nbins = nbins
        self._yscale = yscale
        self._seed = seed

    @staticmethod
    def from_pickle_files(pickle_files: str | Sequence[str], properties: list[dict],
                           model_file: str, plots_file: str, steer: dict | None = None,
                           **kwargs) -> 'ModelDistributionAsFlowAction':
        """Alternate constructor bypassing the CutflowProcessor: loads pre-extracted
        data dumps from one or multiple pickle files instead of pulling data from a
        DataStore. Each file must hold a dict with keys 'data' (dict[str, np.ndarray],
        already forward-transformed, keyed by property name), 'fsc_coded'
        (np.ndarray) and 'conditioning_vector' (np.ndarray) -- the format written by
        dumpStoreData(). Multiple files are concatenated along the event axis.

        Args:
            pickle_files: path(s) to one or multiple pickle dumps.
            properties: same as ModelDistributionAsFlowAction.__init__.
            model_file: same as ModelDistributionAsFlowAction.__init__.
            plots_file: same as ModelDistributionAsFlowAction.__init__.
            steer: optional cutflow steering dict; only used if e.g. `$hypothesis`-style
                path expansion was already applied by the caller. Defaults to {}.
            **kwargs: any other keyword argument of ModelDistributionAsFlowAction.__init__.
        """
        return ModelDistributionAsFlowAction(
            None, steer if steer is not None else {}, properties, model_file, plots_file,
            pickle_files=pickle_files, **kwargs
        )

    def output(self):
        return [self.localTarget(self._model_file), self.localTarget(self._plots_file)]

    # -- data extraction ----------------------------------------------------

    def _resolveSource(self) -> 'DataSource':
        sources = self._cp.getSources()

        if self._source is None:
            return sources[0]
        elif isinstance(self._source, int):
            return sources[self._source]
        else:
            return self._cp.getSource(self._source)

    def _extractFromStore(self) -> tuple[dict[str, np.ndarray], np.ndarray, np.ndarray]:
        from ..DataStore import parse_final_state_counts

        source = self._resolveSource()
        store = source.getStore()
        store.resetView()

        fsc = parse_final_state_counts(store)
        data = extract_data(store, self._properties)
        conditioning_vector = extract_conditioning_vector(fsc, store['process'], store['pol_code'])

        return data, np.asarray(store['fsc_coded']), conditioning_vector

    def _extractFromPickles(self) -> tuple[dict[str, np.ndarray], np.ndarray, np.ndarray]:
        assert self._pickle_files is not None
        feature_names = [prop.name for prop in self._properties]

        data: dict[str, list[np.ndarray]] = {name: [] for name in feature_names}
        fsc_coded_parts: list[np.ndarray] = []
        conditioning_parts: list[np.ndarray] = []

        for file in self._pickle_files:
            with open(file, 'rb') as pf:
                dump = pickle.load(pf)

            missing = [name for name in feature_names if name not in dump['data']]
            if missing:
                raise Exception(f'Pickle dump <{file}> is missing properties {missing}')

            for name in feature_names:
                data[name].append(np.asarray(dump['data'][name]))

            fsc_coded_parts.append(np.asarray(dump['fsc_coded']))
            conditioning_parts.append(np.asarray(dump['conditioning_vector']))

        data_concat = {name: np.concatenate(parts, axis=0) for name, parts in data.items()}
        fsc_coded = np.concatenate(fsc_coded_parts, axis=0)
        conditioning_vector = np.concatenate(conditioning_parts, axis=0)

        return data_concat, fsc_coded, conditioning_vector

    def dumpStoreData(self, file: str):
        """Extracts data from the configured DataStore (the same data run() would
        train on) and writes it to `file` in the format expected by
        from_pickle_files(). Useful to move training onto a machine without access
        to the underlying ROOT/HDF5 data, e.g. a GPU node.
        """
        data, fsc_coded, conditioning_vector = self._extractFromStore()

        os.makedirs(osp.dirname(osp.abspath(file)), exist_ok=True)
        with open(file, 'wb') as pf:
            pickle.dump({
                'data': data,
                'fsc_coded': fsc_coded,
                'conditioning_vector': conditioning_vector,
            }, pf)

    # -- training / control plots ---------------------------------------------

    def run(self):
        if self.complete():
            return

        _require_torch()
        import torch

        if self._seed is not None:
            np.random.seed(self._seed)
            torch.manual_seed(self._seed)

        if self._pickle_files is not None:
            data, fsc_coded, conditioning_vector = self._extractFromPickles()
        else:
            data, fsc_coded, conditioning_vector = self._extractFromStore()

        feature_names = [prop.name for prop in self._properties]

        x_raw = np.stack([data[name] for name in feature_names], axis=1).astype(np.float32)
        finite_mask = np.all(np.isfinite(x_raw), axis=1)
        x_raw = x_raw[finite_mask]
        fsc_coded = fsc_coded[finite_mask]
        conditioning_vector = conditioning_vector[finite_mask].astype(np.float32)

        x_mean = x_raw.mean(axis=0)
        x_std = np.where(x_raw.std(axis=0) == 0, 1.0, x_raw.std(axis=0))
        x_norm = (x_raw - x_mean) / x_std

        cond_norm = None
        cond_mean = cond_std = None
        if self._conditioned:
            cond_mean = conditioning_vector.mean(axis=0)
            cond_std = np.where(conditioning_vector.std(axis=0) == 0, 1.0, conditioning_vector.std(axis=0))
            cond_norm = (conditioning_vector - cond_mean) / cond_std

        device = self._device or ('cuda' if torch.cuda.is_available() else 'cpu')

        model_state = train_cfm(
            x_norm, cond_norm,
            time_dim=self._time_dim, hidden=self._hidden, embed_dim=self._embed_dim,
            epochs=self._epochs, lr=self._lr, batch_size=self._batch_size,
            max_norm=self._max_norm, device=device,
        )

        model_state['feature_names'] = feature_names
        model_state['properties'] = self._properties_raw
        model_state['x_mean'] = x_mean
        model_state['x_std'] = x_std
        model_state['cond_mean'] = cond_mean
        model_state['cond_std'] = cond_std
        model_state['device'] = device

        model_target, plots_target = self.output()
        os.makedirs(model_target.absdirname, exist_ok=True)
        torch.save(model_state, model_target.abspath)

        self._makeControlPlots(model_state, data, fsc_coded, conditioning_vector, device)

    def _makeControlPlots(self, model_state: dict[str, Any], data: dict[str, np.ndarray],
                           fsc_coded: np.ndarray, conditioning_vector: np.ndarray, device: str):
        import torch
        from phc import export_figures
        import matplotlib.pyplot as plt

        velocity_field, cond_embedding = build_flow_model(model_state, device=device)

        n_samples = self._n_control_samples
        ctx = None
        class_mask = np.ones(len(fsc_coded), dtype=bool)

        if self._conditioned:
            fsc_unq, counts = np.unique(fsc_coded, return_counts=True)
            target_class = self._control_plot_class if self._control_plot_class is not None else fsc_unq[np.argmax(counts)]
            class_mask = fsc_coded == target_class

            cond_mean, cond_std = model_state['cond_mean'], model_state['cond_std']
            class_cond = (conditioning_vector[class_mask][0] - cond_mean) / cond_std
            class_cond_tensor = torch.tensor(class_cond, dtype=torch.float32, device=device)

            with torch.no_grad():
                ctx = cond_embedding(class_cond_tensor.unsqueeze(0).repeat(n_samples, 1))

        samples_norm = sample_cfm(velocity_field, n_samples, model_state['n_features'],
                                   ctx=ctx, steps=self._ode_steps, device=device)
        samples = samples_norm.cpu().numpy() * model_state['x_std'] + model_state['x_mean']

        figures = _plot_control_histograms(
            data, samples, class_mask, self._properties, yscale=self._yscale, nbins=self._nbins)

        plots_target = self.output()[1]
        os.makedirs(plots_target.absdirname, exist_ok=True)
        export_figures(plots_target.abspath, figures)

        for fig in figures:
            plt.close(fig)


def _plot_control_histograms(data: dict[str, np.ndarray], samples: np.ndarray, class_mask: np.ndarray,
                              properties: list[LogitNamedProperty], yscale: str = 'log', nbins: int = 64):
    import matplotlib.pyplot as plt
    from matplotlib.figure import Figure

    figures: list[Figure] = []

    for i, prop in enumerate(properties):
        source_vals = prop.backward(data[prop.name][class_mask])
        trained_vals = prop.backward(samples[:, i])

        edges = np.linspace(source_vals.min(), source_vals.max(), nbins)

        # density=True so the two histograms are comparable despite different sample sizes
        source_counts, _ = np.histogram(source_vals, bins=edges, density=True)
        trained_counts, _ = np.histogram(trained_vals, bins=edges, density=True)
        residual = 100 * (trained_counts - source_counts) / np.maximum(source_counts, 1e-10)
        residual = np.clip(residual, -300, 300)  # limit extreme values for better visualization

        fig, (ax_main, ax_res) = plt.subplots(
            nrows=2, sharex=True, figsize=(6, 6),
            gridspec_kw={'height_ratios': [3, 1], 'hspace': 0.05}
        )

        ax_main.stairs(source_counts, edges, label='Source', linewidth=1.5, alpha=0.7)
        ax_main.stairs(trained_counts, edges, label='Trained', linewidth=1.5, alpha=0.7)
        ax_main.set_ylabel('Density')
        ax_main.set_yscale(yscale)
        ax_main.set_title(prop.name)
        ax_main.legend()

        ax_res.axhline(0, color='black', linewidth=0.8)
        ax_res.stairs(residual, edges, fill=True, color='gray')
        ax_res.set_ylabel(r'$\frac{Trained - Source}{Source} [\%]$')
        ax_res.set_xlabel(prop.name)

        fig.set_layout_engine('tight')
        figures.append(fig)

    return figures
