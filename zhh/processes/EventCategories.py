from .event_category_registry import GlobalEventCategoryRegistry, FinalStateDefinition

EventCategories = GlobalEventCategoryRegistry

def find_event_category_definition(name:str)->FinalStateDefinition:
    """Tries to find the definition for the event category given by name in the
    GlobalEventCategoryRegistry.

    Args:
        name (str): _description_

    Raises:
        Exception: _description_

    Returns:
        FinalStateDefinition: _description_
    """

    evt_category_definition = EventCategories.getItemByName(name)

    if evt_category_definition.final_state is None:
        raise Exception(f'No final state definition registered for event category <{name}>')

    return evt_category_definition.final_state