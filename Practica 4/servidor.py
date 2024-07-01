# server.py
from flask import Flask, request, jsonify
import requests

app = Flask(__name__)

@app.route('/fetch', methods=['POST'])
def fetch_page():
    data = request.json
    url = data.get('url')
    
    if not url:
        return jsonify({'error': 'No URL provided'}), 400
    
    try:
        response = requests.get(url)
        response.raise_for_status()
    except requests.exceptions.RequestException as e:
        return jsonify({'error': str(e)}), 400
    
    return jsonify({'html': response.text})

if __name__ == '__main__':
    app.run(debug=True)
