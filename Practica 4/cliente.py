# client.py
import requests

def fetch_page(url):
    response = requests.post('http://127.0.0.1:5000/fetch', json={'url': url})
    if response.status_code == 200:
        return response.json().get('html')
    else:
        print(f"Error: {response.json().get('error')}")
        return None

def save_modified_page(content):
    with open('modified_page.html', 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    url = input("Enter the URL of the page to fetch: ")
    html_content = fetch_page(url)
    
    if html_content:
        print("Original HTML content fetched. You can now modify it.")
        # Simulating modification
        modified_content = html_content.replace('<title>', '<title>Modified: ')
        
        save_modified_page(modified_content)
        print("Modified HTML content saved to 'modified_page.html'.")

if __name__ == '__main__':
    main()
