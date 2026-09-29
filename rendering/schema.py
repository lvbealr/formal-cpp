import json
from pathlib import Path


def read_document(filename):
    document = json.loads(Path(filename).read_text(encoding='utf-8'))
    if not isinstance(document, dict) or document.get('schema_version') != 1:
        raise ValueError('Expected JSON schema_version 1')
    return document


def to_networkx(document):
    import networkx as nx

    kind = document.get('type')
    if kind not in ('nfa', 'dfa'):
        raise ValueError('Expected an nfa or dfa document')
    states = document['states']
    if any(type(state) is not int for state in states) or len(set(states)) != len(states):
        raise ValueError('states must be distinct integers')
    states = set(states)
    alphabet = document['alphabet']
    if any(not isinstance(s, str) or len(s) != 1 for s in alphabet):
        raise ValueError('alphabet must contain one-character strings')
    starts = set(document['initial_states'] if kind == 'nfa' else [document['initial_state']])
    finals = set(document['final_states'])
    if not starts <= states or not finals <= states:
        raise ValueError('Initial and final states must belong to states')
    graph = nx.DiGraph()
    for state in sorted(states):
        graph.add_node(state, initial=state in starts, final=state in finals)
    seen = set()
    for transition in document['transitions']:
        source, symbol = transition['source'], transition['symbol']
        targets = transition['targets'] if kind == 'nfa' else [transition['target']]
        if source not in states or any(target not in states for target in targets):
            raise ValueError('Transition endpoint does not belong to states')
        if not (symbol is None and kind == 'nfa') and symbol not in alphabet:
            raise ValueError('Transition symbol does not belong to alphabet')
        key = (source, symbol)
        if key in seen:
            raise ValueError('Duplicate transition key')
        seen.add(key)
        for target in sorted(set(targets)):
            if not graph.has_edge(source, target):
                graph.add_edge(source, target, symbols=[])
            graph[source][target]['symbols'].append(symbol)
    for _, _, data in graph.edges(data=True):
        data['symbols'].sort(key=lambda s: (s is not None, s or ''))
        data['label'] = ', '.join('ε' if s is None else s for s in data['symbols'])
    return graph


def drawing_options(document):
    data = document.get('drawing', {})
    states = set(document['states'])
    labels = {int(k): str(v) for k, v in data.get('labels', {}).items()}
    positions = {int(k): v for k, v in data.get('positions', {}).items()}
    groups = [set(group) for group in data.get('groups', [])]
    if not set(labels) <= states or any(not group <= states for group in groups):
        raise ValueError('Drawing labels/groups reference unknown states')
    return dict(title=data.get('title') or None, labels=labels,
                positions=positions or None, groups=groups or None,
                layout=data.get('layout', 'layers'))
