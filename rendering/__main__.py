import argparse
from html import escape
from pathlib import Path

from .schema import read_document, drawing_options
from .drawing import draw_automaton


def render_file(source, output, formats=('png', 'svg'), layout=None):
    document = read_document(source)
    options = drawing_options(document)
    if layout is not None:
        options['layout'] = layout
    figure = draw_automaton(document, **options)
    output.mkdir(parents=True, exist_ok=True)
    for extension in formats:
        figure.savefig(output / f'{source.stem}.{extension}', dpi=160, bbox_inches='tight')
    return document


def render(source, output=None, formats=('png', 'svg'), layout=None):
    source = Path(source)
    if source.is_dir():
        source /= 'manifest.json'
    document = read_document(source)
    output = Path(output) if output is not None else source.parent
    if document.get('type') != 'gallery':
        render_file(source, output, formats, layout)
        return output / f'{source.stem}.{formats[0]}'
    cards, navigation = [], []
    for item in document['items']:
        filename = item['file']
        if Path(filename).name != filename or not filename.endswith('.json'):
            raise ValueError('Gallery items must name JSON files in the same directory')
        file = source.parent / filename
        automaton = render_file(file, output, formats, layout)
        title = automaton.get('drawing', {}).get('title') or item.get('title', file.stem)
        notes = automaton.get('drawing', {}).get('notes', '') + '\n\n' + automaton.get('table', '')
        name = escape(file.stem, quote=True)
        title = escape(title)
        preview = 'svg' if 'svg' in formats else formats[0]
        links = ' · '.join(f'<a href="{name}.{ext}">{ext.upper()}</a>' for ext in formats)
        cards.append(f'<section id="{name}"><h2>{title}</h2>{links}'
                     f'<a href="{name}.{preview}"><img src="{name}.{preview}" alt="{title}" loading="lazy"></a>'
                     f'<details><summary>Таблица переходов и пояснения</summary><pre>{escape(notes.strip())}</pre></details></section>')
        navigation.append(f'<a href="#{name}">{title}</a>')
        print(f'Сохранён: {file.stem}', flush=True)
    title = escape(document.get('title', 'Автоматы'))
    html = f'''<!doctype html>
<html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title><style>
body {{margin:0;background:#f4f6fa;color:#18263b;font:16px/1.6 system-ui,sans-serif}}
main {{max-width:1400px;margin:auto;padding:32px}} h2 {{font-size:22px;margin:0}}
a {{color:#205bc0}} nav {{display:flex;gap:8px 20px;flex-wrap:wrap;padding:20px 0}}
nav a {{font-size:14px}} section {{background:white;padding:24px;margin:24px 0;border:1px solid #dfe5ee;border-radius:12px}}
img {{display:block;width:100%;height:auto;margin:12px 0}} pre {{overflow:auto;font-size:13px}}
summary {{cursor:pointer}} @media(max-width:650px) {{main,section {{padding:12px}}}}
</style><main><h1>{title}</h1>
<p>Синяя входящая стрелка — старт. Двойной круг — финал. ε — переход без чтения буквы.
B0, B1, … — блоки разбиения. Нажмите на рисунок для увеличения.</p>
<nav>{''.join(navigation)}</nav>{''.join(cards)}</main></html>'''
    output.mkdir(parents=True, exist_ok=True)
    gallery = output / 'index.html'
    gallery.write_text(html, encoding='utf-8')
    return gallery


def main():
    parser = argparse.ArgumentParser(description='Рендеринг JSON из C++ через NetworkX и Matplotlib')
    parser.add_argument('source', type=Path, help='Файл автомата, manifest.json или каталог галереи')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--format', choices=('both', 'png', 'svg'), default='both')
    parser.add_argument('--layout', choices=('layers', 'spring', 'circular'))
    args = parser.parse_args()
    import matplotlib
    matplotlib.use('Agg')
    try:
        formats = ('png', 'svg') if args.format == 'both' else (args.format,)
        result = render(args.source, args.output, formats, args.layout)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f'Ошибка: {error}\n')
    print(f'Готово: {result.resolve()}')


if __name__ == '__main__':
    main()
