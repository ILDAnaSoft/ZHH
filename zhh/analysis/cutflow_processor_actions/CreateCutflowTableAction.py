from typing import Sequence, cast
import os.path as osp
import numpy as np

from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from ..Cuts import Cut
from ..DataSource import DataSource

class CreateCutflowTableAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, weight_columns:list[str]|None=None, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            file (str): _description_
            weight_columns (list[str]): _description_
        """

        from zhh import CategorizedCutflowTableEntry

        assert('cutflow_table' in steer)

        super().__init__(cp, steer)
        
        self._file = file
        self._step_start = kwargs.get('step_start', 0)
        self._step_end = kwargs.get('step_end', 0)
        self._weight_columns = weight_columns
        self._steer = steer

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

        from zhh.analysis.CutflowProcessor import cutflowTableFn

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
                       path=str(self.output()[0].abspath))
    
    def output(self):
        return [
            self.localTarget(f'{osp.splitext(self._file)[0]}.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_efficiency.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_frac.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_counts.csv')
        ]
    
def parse_cutflow_table_entries(steer:dict):
    from zhh import CutflowTableEntry, LatexCutflowTableEntry, SumCutflowTableEntry, \
        CategorizedCutflowTableEntry, UncategorizedCutflowTableEntry
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

def match_sources_to_categories(sources:list[DataSource], categories:list[str])->dict[str, list[str]]:
    """For each category A in event_categories, finds a source in sources in which
    category A is registered. Throws an exception if no source can be found for A.

    Returns a dict of structure category => source_name.

    Args:
        sources (list[DataSource]): _description_
        categories (list[str]): _description_

    Raises:
        Exception: _description_

    Returns:
        dict[str, list[str]]: _description_
    """
    source_to_category_names:dict[str, list[str]] = {}

    for category in categories:
        found = False

        for source in sources:
            if source.containsCategory(category):
                if source.getName() not in source_to_category_names:
                    source_to_category_names[source.getName()] = []
                
                source_to_category_names[source.getName()] += [category]

                found = True
                break

        if not found:
            raise Exception(f'Category <{category}> is unknown in all registered sources')

    return source_to_category_names

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

    # delete 0 entries in source_2_category_2_count[source.getName()][category]
    for category in ordered_categories.keys():
        if categories_2_count[category].sum() == 0:
            print(f'Deleting count for {category} in {source.getName()} because it\'s 0')
            del categories_2_count[category]

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

    source_2_category_names = match_sources_to_categories(sources, all_categories)

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