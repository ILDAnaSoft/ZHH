from typing import Sequence, cast
import os.path as osp
import numpy as np

from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from ..Cuts import Cut
from ..DataSource import DataSource
from ..CutflowTableEntry import CutflowTableEntry, LatexCutflowTableEntry, SumCutflowTableEntry, \
    UncategorizedCutflowTableEntry, CategorizedCutflowTableEntry

class CreateCutflowTableAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, weight_columns:list[str]|None=None,
                 show_cross_sections:bool=True, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            file (str): _description_
            weight_columns (list[str]): _description_
            show_cross_sections (bool): if True (default), the table of absolute counts gets a column with the
                cross section in fb (expected events before cuts / integrated luminosity)
        """

        assert('cutflow_table' in steer)

        super().__init__(cp, steer)
        
        self._file = file
        self._step_start = kwargs.get('step_start', 0)
        self._step_end = kwargs.get('step_end', 0)
        self._weight_columns = weight_columns
        self._steer = steer
        self._show_cross_sections = show_cross_sections

        self._cutflow_table_entries = parse_cutflow_table_entries(steer)
        self._all_categories:list[str] = [cast(CategorizedCutflowTableEntry, a).category for a in list(
            filter(lambda a: isinstance(a, CategorizedCutflowTableEntry), self._cutflow_table_entries))]
        
        signal_categories = []
        for entry in self._cutflow_table_entries:
            if isinstance(entry, CategorizedCutflowTableEntry):
                if entry.is_signal:
                    signal_categories.append(entry.category)

        self._signal_categories = signal_categories

        # check whether the requested event categories exist in the registered sources 
        get_sources_2_categories(steer, cp._sources, self._all_categories)
    
    def run(self):
        weight_columns:list[str] = [self._cp._weight_columns[step] for step in range(self._step_start, self._step_end+1)] if self._weight_columns is None else self._weight_columns

        masks = []
        cuts = []

        for step in range(self._step_start, self._step_end+1):
            masks.append(self._cp._masks[step])
            cuts.append(self._cp._cuts[step])
        
        source_2_counts, source_2_category_names = calculate_counts_by_category(
            self._steer, masks, self._cp._sources, self._all_categories, cuts, weight_columns)

        cutflowTableFn(source_2_counts,
                       source_2_category_names,
                       self._signal_categories,
                       luminosity=self._steer['luminosity'],
                       cutflow_table_entries=self._cutflow_table_entries,
                       cuts=cuts,
                       path=str(self.output()[0].abspath),
                       show_cross_sections=self._show_cross_sections)
    
    def output(self):
        return [
            self.localTarget(f'{osp.splitext(self._file)[0]}.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_efficiency.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_frac.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_counts.csv')
        ]
    
def parse_cutflow_table_entries(steer:dict):
    from copy import deepcopy

    cutflow_table_entries:Sequence[CutflowTableEntry|LatexCutflowTableEntry] = []

    for item in steer['cutflow_table']['items']:
        if 'category' in item:
            entry = CategorizedCutflowTableEntry(**item)
        elif 'remaining_of_source' in item:
            args = deepcopy(item)
            args['source'] = args['remaining_of_source']

            del args['remaining_of_source']
            entry = UncategorizedCutflowTableEntry(**args)
        elif 'latex' in item:
            entry = LatexCutflowTableEntry(item['latex'])
        elif 'sum' in item:
            entry = SumCutflowTableEntry(**item)
        else:
            print(item)
            raise Exception('Cannot parse to CutflowTableEntries')

        cutflow_table_entries += [entry]
    
    return cutflow_table_entries

def counts_by_category(masks:list[list[dict[str, np.ndarray]]],
                       cuts:Sequence[Sequence[Cut]],
                       source:DataSource,
                       categories:list[str],
                       weight_columns:list[str],
                       weight_column_initial:str|None=None)->dict[str, np.ndarray]:
    """Fetches the number of events passing a list of cut groups.
    masks, cuts and weight_columns must have the same number of entries (i.e. cut groups).
    Within each cut j in cut_group i, masks[i][j] must be a dict[str, np.ndarray] where
    the key is the name of a source and the value is a binary mask of events passing the
    the cut j. There must be a value for the given source.
    categories is a list of event categories for the given source to calculate the event
    count for (at each cut j). To calculate the event count, event weights from the
    column weight_columns[i] are used and the label is inferred frm cuts[i][j].
    The output is a dict[str, np.ndaray] where the key is the category and the value a
    numpy array with size=(Sum[1] over j,i), i.e. total number of cuts.
    
    Note: the count _before_ cuts is not calculated by this function. 

    Args:
        masks (list[list[dict[str, np.ndarray]]]): _description_
        cuts (list[list[Cut]]): _description_
        source (DataSource): _description_
        categories (list[str]): _description_
        weight_columns (list[str]): _description_

    Returns:
        dict[str, np.ndarray]: _description_
    """

    from zhh import evaluate_categories_ordered

    if weight_column_initial is None:
        weight_column_initial = weight_columns[0]

    n_cuts = 0
    for i, cut_group in enumerate(cuts):
        n_cuts += len(cut_group)

    n_categories_tot = 0

    # if all categories registered in steer[source].items were also plotted, this would be faster:
    # evaluate_categories(source, categories, 'event_category')

    ordered_categories = evaluate_categories_ordered(source, categories)
    n_categories = len(ordered_categories.keys())

    categories_2_count:dict[str, np.ndarray] = {}
    for category in ordered_categories:
        categories_2_count[category] = np.zeros(n_cuts + 1)

    n_categories_tot += n_categories

    # fill initial count
    weights = source.getStore()[weight_column_initial]

    for i, cut_mask_group in enumerate(masks):
        for k, category in enumerate(ordered_categories):
            category_mask = ordered_categories[category]
            categories_2_count[category][0] = weights[category_mask].sum()

    # fill the counts in source_2_category_2_count per source/category and cut
    i_cut = 0
    for i, cut_mask_group in enumerate(masks):
        weight_prop = weight_columns[i]
        weights = source.getStore()[weight_prop]

        for j, cut_masks in enumerate(cut_mask_group):
            cut = cuts[i][j]
            found = False

            for src_name, cut_mask in cut_masks:
                if src_name == source.getName():
                    found = True
                    break
            
            assert(found)

            for k, category in enumerate(ordered_categories):
                category_mask = ordered_categories[category]
                categories_2_count[category][i_cut + 1] = weights[cut_mask & category_mask].sum()
        
            i_cut += 1

    # print if 0 entries in source_2_category_2_count[source.getName()][category] found
    for category in ordered_categories.keys():
        if categories_2_count[category].sum() == 0:
            print(f'Warning: Found 0 count for {category} in {source.getName()}. Are you sure the ordering of categories is correct?')
    #        del categories_2_count[category]

    return categories_2_count

def find_source_spec(steer:dict, source_name:str):
    found = False
    
    for source_spec in steer['sources']:
        if source_spec['name'] == source_name:
            found = True
            break

    if not found:
        raise Exception(f'Could not find source <{source_name}> in steer')

    return source_spec

def get_sources_2_categories(steer:dict, sources:list[DataSource], all_categories:list[str]):
    """Given a list of categories and DataSource items and a steering dict, creates a
    dict[str, list[str]] of structure source => category_names where category_names is
    sorted in order they appear in steer['sources']['event_categorization'] at key order
    or items. The ordering is important when apply the event categories one after each
    other.

    Args:
        steer (dict): the steering from which to infer the event_categorization
        sources (list[DataSource]): sources in which to look for the categories in all_categories 
        all_categories (list[str]): _description_

    Raises:
        Exception: _description_

    Returns:
        _type_: _description_
    """

    source_2_category_names = DataSource.match_sources_to_categories(sources, all_categories)

    # sort source_2_category_names[source]
    for source in list(source_2_category_names.keys()):
        categorization = find_source_spec(steer, source)['event_categorization']
        order = categorization['order'] if (
                    categorization['order'] is not None and len(categorization['order']) == len(categorization['items'])
                ) else categorization['items']

        categories_unsorted = source_2_category_names[source]

        for category in categories_unsorted:
            if category not in order:
                raise Exception(f'Could not find category {category} for source {source}. Is it properly registered?')

        categories_sorted = list(sorted(categories_unsorted, key=lambda x: order.index(x)))
        source_2_category_names[source] = categories_sorted

    # account for the case that no category was requested for a source --> it does not exist in source_2_category_names
    for source in sources:
        if not source.getName() in source_2_category_names.keys():
            source_2_category_names[source.getName()] = []
    
    return source_2_category_names

def calculate_counts_by_category(
        steer:dict,
        masks,
        sources:list[DataSource],
        all_categories:list[str],
        cuts:Sequence[Sequence[Cut]],
        weight_columns:list[str]):
    
    from tqdm.auto import tqdm

    source_2_category_names = get_sources_2_categories(steer, sources, all_categories)

    # prepare source_2_counts
    source_2_counts = {}

    for source in (pbar := tqdm(sources)):
        pbar.set_description(f'Calculating event count for source {source.getName()} with categories {", ".join(source_2_category_names[source.getName()])}')
        source_2_counts[source.getName()] = counts_by_category(masks, cuts, source, source_2_category_names[source.getName()], weight_columns=weight_columns)
    
    return source_2_counts, source_2_category_names

def cutflowTableFn(source_2_counts:dict[str, dict[str, np.ndarray]],
                   source_2_category_names:dict[str, list[str]],
                   signal_categories:list[str],
                   luminosity:float,
                   cutflow_table_entries:Sequence[CutflowTableEntry|LatexCutflowTableEntry],
                   cuts:Sequence[Sequence[Cut]],
                   path:str,
                   show_cross_sections:bool=True):
    """Renders the cutflow tables (absolute counts, efficiencies, fraction passing).

    Args:
        luminosity (float): integrated luminosity in ab^-1
        show_cross_sections (bool): if True, the table of absolute counts gets an additional
            column with the (effective, i.e. polarization weighted) cross section in fb, calculated
            as the expected number of events before any cut divided by the luminosity
    """
    
    from zhh import combined_cross_section, renderTableFn, renderLatexFn, EventCategories, \
        LatexRenderContext
    from ..CutflowProcessor import invert_dict, format_ndigits
    from tqdm.auto import tqdm

    category_names_2_source = invert_dict(source_2_category_names)
    
    first_item = source_2_counts[list(source_2_counts.keys())[0]]
    first_item = first_item[list(first_item.keys())[0]]

    entry_counts = np.zeros((len(cutflow_table_entries), len(first_item)))
    entry_efficiencies = np.zeros((len(cutflow_table_entries), len(first_item) - 1))
    entry_passing_frac = np.zeros((len(cutflow_table_entries), len(first_item) - 1))
    category_counts:dict[str, np.ndarray] = {}

    def xsec_from_counts(n_events:int|float)->list[str]:
        """Returns a list with either one (the formatted cross-section)
        or no item for convenient printing.

        Args:
            n_events (int | float): _description_

        Returns:
            list[str]: _description_
        """
        return [ format_ndigits(n_events / (luminosity * 1000.)) ] if show_cross_sections else []

    csv_out = f'{osp.splitext(path)[0]}_counts.csv'

    render_context = LatexRenderContext(work_dir=f'{osp.dirname(csv_out)}/latex-build-{osp.splitext(osp.basename(path))[0]}', packages=[ 'ydoc', 'standalone', 'upgreek' ])

    for n_run in range(3):
        render_abs_table = n_run == 0
        render_eff_table = n_run == 1
        render_frac_table = n_run == 2

        if render_eff_table:
            for i in range(entry_counts.shape[1] - 1):
                entry_efficiencies[:, i] = entry_counts[:, i+1] / entry_counts[:, i]
                entry_passing_frac[:, i] = entry_counts[:, i+1] / entry_counts[:, 0]

        entries = (entry_counts if render_abs_table else (entry_efficiencies if render_eff_table else entry_passing_frac))
        out_name = osp.splitext(path)[0] +('.pdf' if render_abs_table else ('_efficiency.pdf' if render_eff_table else '_frac.pdf'))

        table = []
        header = ['']

        if render_abs_table:
            if show_cross_sections:
                header.append(r'$\sigma$ [fb]')

            header.append('expected')

        for cut_group in cuts:
            for cut in cut_group:
                header += [f'${cut.latex()}$']

        table.append(r'\hline') # line separating header and body

        # each entry in row must either be a str or a list of n strings (where n equal for all lists)

        for i, entry in enumerate(cutflow_table_entries):
            category_out:str|None = None

            if isinstance(entry, CategorizedCutflowTableEntry):
                category = entry.category
                source_name = category_names_2_source[category]

                if render_abs_table:
                    #print(source_name, category, source_2_counts[source_name].keys())
                    entry_counts[i, :] = source_2_counts[source_name][category]
                    category_out = f'{source_name}.{category}'

                    table += [[ entry.label, *xsec_from_counts(entries[i, 0]), *[format_ndigits(a) for a in entries[i, :] ] ]]
                else:
                    table += [[ entry.label, *[f'{a:.2%}'.replace('%', r'\%') for a in entries[i, :] ] ]]
            elif isinstance(entry, UncategorizedCutflowTableEntry):
                source_name = entry.source
                
                if render_abs_table:
                    entry_counts[i, :] = source_2_counts[source_name]['other']
                    category_out = f'{source_name}.other'

                    table += [[ entry.label, *xsec_from_counts(entries[i, 0]), *[format_ndigits(a) for a in entries[i, :] ] ]]
                else:
                    table += [[ entry.label, *[f'{a:.2%}'.replace('%', r'\%') for a in entries[i, :] ] ]]
            elif isinstance(entry, LatexCutflowTableEntry):
                table += [entry.latex]
            elif isinstance(entry, SumCutflowTableEntry):
                cats = entry.sum
                counts = np.zeros(entry_counts.shape[1])

                if isinstance(cats, str):
                    if cats.lower() in ['signal', 'background']:
                        count_signal = cats.lower() == 'signal'
                        
                        for source, source_counts in source_2_counts.items():
                            for category, count in source_counts.items():
                                if (count_signal and category in signal_categories) or (
                                    not count_signal and category not in signal_categories):
                                    counts += count
                    else:
                        raise Exception(f'Cannot parse <{cats}>')
                    
                    category_out = f'total_{cats}'
                else:
                    for category in cats:
                        if '.' in category:
                            split_items = '.'.split(category)
                            source, cat = split_items[0], split_items[1]
                            counts += source_2_counts[source][cat]
                        else:
                            source = category_names_2_source[category]
                            counts += source_2_counts[source][category]

                    category_out = '_and_'.join(cats)

                if render_abs_table:
                    entry_counts[i, :] = counts
                    
                    table += [[ entry.label, *xsec_from_counts(counts[0]), *[format_ndigits(a) for a in counts ]]]
                else:
                    table += [[ entry.label, *[f'{a:.2%}'.replace('%', r'\%') for a in entries[i, :] ]]]
            else:
                print(entry)
                raise Exception(f'Received non-parseable item <{entry.__class__.__name__}>')
            
            if render_abs_table and category_out is not None and entry_counts[i, :].sum():
                category_counts[category_out] = entry_counts[i, :]

        table.insert(0, header)
        #lable = transpose(table)

        latex_out = renderTableFn(table)
        
        print(out_name, latex_out)

        render_context.render(latex_out, out_name)
    
    # write out CSV file with counts
    with open(csv_out, 'tw') as cf:
        cf.write(','.join(header))
        for cat, counts in category_counts.items():
            cf.write(f'\n{cat}')
            for count in counts:
                cf.write(f',{count}')
                #cf.write(f',{count:.6g}') # round to 6 significant digits