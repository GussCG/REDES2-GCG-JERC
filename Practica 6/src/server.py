from flask import Flask, render_template, request, jsonify
import requests

app = Flask(__name__)

TPB_API_URL = 'https://apibay.org/q.php?q={}'

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/search')
def search():
    query = request.args.get('query')
    try:
        page = int(request.args.get('page', 1))
    except ValueError:
        page = 1

    results_per_page = 6
    start = results_per_page * (page - 1)
    end = start + results_per_page
    
    response = requests.get(TPB_API_URL.format(query))
    if response.status_code == 200:
        results = response.json()
        torrents = []
        for result in results[start:end]:
            torrents.append({
                'name': result['name'],
                'size': result['size'],
                'magnet_url': f"magnet:?xt=urn:btih:{result['info_hash']}&dn={result['name']}",
            })
        return jsonify(torrents), 200
    else:
        return jsonify({'error': 'Error al obtener los torrents'}), 500
    
@app.route('/results')
def results():
    return render_template('search.html')
    
if __name__ == '__main__':
    app.run(debug=True)