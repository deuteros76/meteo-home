import os
import re
import shutil
from pathlib import Path

def sanitize_filename(title):
    """
    Convierte un título en un nombre de archivo válido.
    Elimina caracteres especiales y espacios en blanco.
    """
    # Elimina caracteres especiales y convierte espacios en guiones
    filename = re.sub(r'[^\w\s-]', '', title.lower())
    filename = re.sub(r'[-\s]+', '-', filename).strip('-')
    return filename + '.md'

def create_front_matter(title):
    """
    Crea el front matter de Jekyll con el layout y título especificados
    """
    return f"""---
layout: page
title: {title}
---
"""

def process_list_items(content):
    """
    Procesa el contenido añadiendo la clase .list a los elementos de las listas
    """
    lines = content.split('\n')
    processed_lines = []
    for line in lines:
        # Añade {:  .list} a las líneas que son elementos de lista
        if line.strip().startswith('- ') or line.strip().startswith('* '):
            # Remove the original bullet point marker when adding the Kramdown class
            processed_lines.append(f"- {{:  .list}} {line.strip()[2:].strip()}")
        else:
            processed_lines.append(line)
    return '\n'.join(processed_lines)

def extract_sections(markdown_content):
    """
    Extrae las secciones de nivel 2 del contenido markdown.
    Retorna un diccionario con los títulos como claves y el contenido como valores.
    """
    sections = {}
    current_title = None
    current_content = []
    lines = markdown_content.split('\n')
    for line in lines:
        # Detecta títulos de nivel 2
        if line.startswith('## '):
            if current_title:
                # Guarda la sección anterior, excluyendo la primera línea (título)
                sections[current_title] = '\n'.join(current_content[1:])
            # Inicia nueva sección
            current_title = line[3:].strip()
            current_content = [line]
        elif current_title:
            # Añade línea al contenido actual
            current_content.append(line)
    # Guarda la última sección, excluyendo la primera línea (título)
    if current_title:
        sections[current_title] = '\n'.join(current_content[1:])
    return sections

def process_markdown_file(input_file):
    """
    Procesa el archivo markdown y crea los archivos correspondientes.
    """
    # Crear directorio docs/_blocks si no existe
    docs_dir = Path('docs/_blocks')
    docs_dir.mkdir(parents=True, exist_ok=True)

    # Leer archivo de entrada
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: No se encontró el archivo {input_file}")
        return
    except Exception as e:
        print(f"Error al leer el archivo: {e}")
        return

    # Extraer secciones
    sections = extract_sections(content)

    # Procesar cada sección
    for title, section_content in sections.items():
        # Crear nombre de archivo
        filename = sanitize_filename(title)
        file_path = docs_dir / filename

        # Procesar contenido añadiendo clase .list
        processed_content = process_list_items(section_content)

        # Crear nuevo archivo con front matter
        try:
            with open(file_path, 'w', encoding='utf-8') as f:
                # Primero escribimos el front matter
                f.write(create_front_matter(title))
                # Luego el contenido de la sección procesado
                f.write(processed_content.strip() + '\n')
            print(f"Archivo creado: {file_path}")
        except Exception as e:
            print(f"Error al crear el archivo {filename}: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Uso: python script.py <archivo_markdown>")
        sys.exit(1)

    input_file = sys.argv[1]
    process_markdown_file(input_file)
