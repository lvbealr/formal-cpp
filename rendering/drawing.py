from pathlib import Path

from .schema import to_networkx


def _layer_positions(graph):
    depth = {}
    queue = []
    for state in graph.nodes:
        if graph.nodes[state]['initial']:
            depth[state] = 0
            queue.append(state)

    position = 0
    while len(depth) < len(graph) or position < len(queue):
        if position == len(queue):
            state = next(state for state in graph.nodes if state not in depth)
            depth[state] = max(depth.values(), default=-1) + 1
            queue.append(state)
        state = queue[position]
        position += 1
        for target in graph.successors(state):
            if target not in depth:
                depth[target] = depth[state] + 1
                queue.append(target)

    layers = {}
    for state in graph.nodes:
        layers.setdefault(depth[state], []).append(state)
    positions = {}
    for layer, states in layers.items():
        for row, state in enumerate(states):
            positions[state] = (1.8 * layer, 1.6 * ((len(states) - 1) / 2 - row))
    return positions


def draw_automaton(automaton, filename=None, title=None, labels=None,
                   layout='layers', show=False, groups=None, positions=None):
    """groups — блоки разбиения, positions — координаты {state: (x, y)}."""
    import networkx as nx
    import matplotlib.pyplot as plt

    graph = to_networkx(automaton)
    if labels is None:
        labels = {}
    if positions is None:
        if layout == 'layers':
            positions = _layer_positions(graph)
        elif layout == 'spring':
            positions = nx.spring_layout(graph, seed=42, scale=max(2, len(graph) ** 0.5))

            distances = []
            states = list(graph.nodes)
            for i, left in enumerate(states):
                for right in states[i + 1:]:
                    dx = positions[left][0] - positions[right][0]
                    dy = positions[left][1] - positions[right][1]
                    distances.append((dx * dx + dy * dy) ** 0.5)
            nearest = min(distances, default=1.2)
            factor = max(1, 1.2 / max(nearest, 0.01))
            positions = {state: (point[0] * factor, point[1] * factor)
                         for state, point in positions.items()}
        elif layout == 'circular':
            positions = nx.circular_layout(graph, scale=max(2, len(graph) / 3))
        else:
            raise ValueError("layout: 'layers', 'spring' или 'circular'")

    if set(positions) != set(graph.nodes):
        raise ValueError('positions must specify every state exactly once')
    xs = [point[0] for point in positions.values()] or [0]
    ys = [point[1] for point in positions.values()] or [0]
    width = max(8, (max(xs) - min(xs)) * 1.1 + 3)
    height = max(5, (max(ys) - min(ys)) * 1.1 + 3)
    figure, axes = plt.subplots(figsize=(width, height))
    figure.set_facecolor('white')
    axes.set_aspect('equal')
    axes.set_xlim(min(xs) - 1.1, max(xs) + 1.0)
    axes.set_ylim(min(ys) - 1.0, max(ys) + 1.4)
    axes.set_axis_off()
    axes.set_title(title or ('НКА' if automaton['type'] == 'nfa' else 'ДКА'),
                   fontsize=17, fontweight='bold', pad=22)

    node_size = 1000
    palette = ['#dbeafe', '#dcfce7', '#fef3c7', '#fce7f3', '#ede9fe', '#cffafe',
               '#fed7aa', '#e2e8f0', '#e9fccb', '#f5d0fe', '#fecdd3', '#ccfbf1']
    colors = {state: '#edf2f7' for state in graph.nodes}
    block_numbers = {}
    for state in graph.nodes:
        if graph.nodes[state]['final']:
            colors[state] = '#dcfce7'
    if groups is not None:
        for number, group in enumerate(groups):
            for state in group:
                colors[state] = palette[number % len(palette)]
                block_numbers[state] = number

    if graph:
        nx.draw_networkx_nodes(graph, positions, node_size=node_size,
                               node_color=[colors[state] for state in graph.nodes],
                               edgecolors='#27364b', linewidths=1.6, ax=axes)
        finals = [state for state in graph.nodes if graph.nodes[state]['final']]

        if finals:
            nx.draw_networkx_nodes(graph, positions, nodelist=finals, node_size=730,
                                   node_color='none', edgecolors='#27364b',
                                   linewidths=1.2, ax=axes)
        text = {state: str(labels.get(state, state)) for state in graph.nodes}
        for state, number in block_numbers.items():
            text[state] += f'\nB{number}'
        nx.draw_networkx_labels(graph, positions, labels=text, font_size=10, ax=axes)

        for source, target in graph.edges:

            radius = 0.20 if source != target and graph.has_edge(target, source) else 0.08
            dx = positions[target][0] - positions[source][0]
            dy = positions[target][1] - positions[source][1]
            if source != target and abs(dx) < 0.01:
                radius = 0.45 if abs(dy) > 2 else 0.25
            elif abs(dx) > 2.5 and abs(dy) < 0.01:
                radius = 0.30
            style = f'arc3,rad={radius}'
            nx.draw_networkx_edges(graph, positions, edgelist=[(source, target)],
                                   node_size=node_size, arrows=True, arrowsize=19,
                                   edge_color='#64748b', width=1.4,
                                   connectionstyle=style, ax=axes)
            nx.draw_networkx_edge_labels(
                graph, positions, edge_labels={(source, target): graph[source][target]['label']},
                node_size=node_size, connectionstyle=style, rotate=False,
                label_pos=0.35 if abs(dx) > 0.01 and abs(dy) > 0.01 else 0.5,
                font_size=10, font_color='#334155',
                bbox={'facecolor': 'white', 'edgecolor': 'none', 'pad': 1.5}, ax=axes,
            )

        for state in graph.nodes:
            if graph.nodes[state]['initial']:
                x, y = positions[state]

                axes.annotate('', xy=(x, y), xytext=(-58, -25),
                              textcoords='offset points',
                              arrowprops={'arrowstyle': '-|>', 'color': '#2563eb',
                                          'lw': 2, 'shrinkB': 18, 'mutation_scale': 19})
    else:
        axes.text(0, 0, 'Нет состояний — язык пуст', ha='center', fontsize=13)

    footer = 'Входящая синяя стрелка — старт  ·  Двойной круг — финал  ·  ε — пустой переход'
    if groups is not None:
        footer += '\nОдинаковый цвет — один блок разбиения; номера блоков указаны в подписях'
    figure.text(0.5, 0.025, footer, ha='center', fontsize=9, color='#475569')
    figure.tight_layout(rect=(0, 0.08, 1, 1))
    if filename is not None:
        path = Path(filename)
        path.parent.mkdir(parents=True, exist_ok=True)
        figure.savefig(path, dpi=160, bbox_inches='tight')
    if show:
        plt.show()
    else:

        plt.close(figure)
    return figure
