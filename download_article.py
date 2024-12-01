import sys
import requests
from bs4 import BeautifulSoup
import os
import markdown
import urllib.parse
from PIL import Image
from io import BytesIO
import html2text
from datetime import datetime
import re

def download_and_convert(url):
    response = requests.get(url)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # Get publication date from article
    pub_date_element = soup.find('dd', class_='published').find('time')
    if pub_date_element and pub_date_element.get('datetime'):
        pub_date = datetime.fromisoformat(pub_date_element['datetime'].replace('Z', '+00:00'))
    else:
        pub_date = datetime.now()  # Fallback to current date if not found

    # Get title from h1
    h1_tag = soup.find('h1')
    title = h1_tag.text.strip() if h1_tag else "Untitled"
    
    # Convert title to URL-friendly slug
    slug = title.lower()
    slug = re.sub(r'[^a-z0-9\s-]', '', slug)
    slug = re.sub(r'\s+', '-', slug)
    
    # Get current date for filename
    current_date = datetime.now().strftime('%Y-%m-%d')
    
    # Create filename with date and slug
    md_filename = f"{pub_date.strftime('%Y-%m-%d')}-{slug}.md"
    
    # Ensure docs/_posts directory exists
    if not os.path.exists('docs/_posts'):
        os.makedirs('docs/_posts')
    
    # Full path for the new file
    full_path = os.path.join('docs/_posts', md_filename)
    
    # Rest of your existing image handling code remains the same
    article_body = soup.find('div', class_='com-content-article__body')
    if article_body:
        content = str(article_body)
    else:
        content = str(soup)
    
    figures = soup.find_all('figure')
    downloaded_images = []
    
    for figure in figures:
        img = figure.find('img')
        try:
            if img and hasattr(img, 'attrs') and 'src' in img.attrs:
                img_url = urllib.parse.urljoin(url, img['src'])
                img_response = requests.get(img_url, verify=False)
                img_name = os.path.basename(img_url)
                
                if not os.path.exists('docs/assets/images/blog'):
                    os.makedirs('docs/assets/images/blog')
                img_path = os.path.join('docs/assets/images/blog', img_name)
                with open(img_path, 'wb') as img_file:
                    img_file.write(img_response.content)
                downloaded_images.append(img_path)
                if len(downloaded_images) == 1:  # For the first image only
                    try:
                        # Open the original image
                        with Image.open(img_path) as img:
                            # Convert to RGB if needed
                            if img.mode != 'RGB':
                                img = img.convert('RGB')
                            
                            # Calculate center crop coordinates for region of interest
                            width, height = img.size
                            crop_size = min(width, height)
                            left = (width - crop_size) // 2
                            top = (height - crop_size) // 2
                            right = left + crop_size
                            bottom = top + crop_size
                            
                            # Crop and resize
                            thumbnail = img.crop((left, top, right, bottom))
                            thumbnail.thumbnail((70, 70), Image.Resampling.LANCZOS)
                            
                            # Create filename from title
                            thumb_filename = f"{slug}-thumb.jpg"
                            thumb_path = os.path.join('docs/assets/images/blog', thumb_filename)
                            thumbnail.save(thumb_path, 'JPEG', quality=85)
                            print(f"Thumbnail created: {thumb_path}")
                    
                    except Exception as e:
                        print(f"Warning: Could not create thumbnail: {str(e)}")
        except Exception as e:
            print(f"Warning: Could not process figure image: {str(e)}")
    
    h = html2text.HTML2Text()
    h.ignore_links = False
    h.ignore_images = False
    
    with open(full_path, 'w', encoding='utf-8') as f:
        f.write('---\n')
        f.write('layout: post\n')
        f.write(f'title: "{title}"\n')
        f.write(f'date: {pub_date.strftime("%Y-%m-%d %H:%M:%S +0100")}\n')
        f.write(f'description: "{h.handle(content)[0:200]}..."\n')
        f.write('---\n\n')
        
        for img_path in downloaded_images:
            f.write(f'![{os.path.basename(img_path)}]({img_path})\n\n')
        f.write(h.handle(content))
    
    print(f"Article successfully converted and saved to {full_path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python download_article.py <url>")
        sys.exit(1)
        
    download_and_convert(sys.argv[1])

