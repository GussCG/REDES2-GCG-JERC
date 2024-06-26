from flask import Flask, request, jsonify, send_file
from datetime import datetime
import os

app = Flask(__name__)

@app.route('/')
def index():
    return send_file('index.html')

@app.route('/submit-get', methods=['GET'])
def handle_get():
    nombre = request.args.get('nombre')
    apellidos = request.args.getlist('apellidos')
    boleta = request.args.get('boleta')
    grupo = request.args.get('grupo')
    return f"""
    <html>
        <body>
            <h1>Parametros Obtenidos por el Metodo GET</h1>
            <p>Nombre: {nombre}</p>
            <p>Apellidos: {', '.join(apellidos)}</p>
            <p>Boleta: {boleta}</p>
            <p>Grupo: {grupo}</p>
        </body>
    </html>
    """

@app.route('/submit-post', methods=['POST'])
def handle_post():
    nombre = request.form['nombre']
    apellidos = request.form.getlist('apellidos')
    boleta = request.form['boleta']
    grupo = request.form['grupo']
    return f"""
    <html>
        <body>
            <h1>Parametros Obtenidos mediante el Metodo POST</h1>
            <p>Nombre: {nombre}</p>
            <p>Apellidos: {', '.join(apellidos)}</p>
            <p>Boleta: {boleta}</p>
            <p>Grupo: {grupo}</p>
        </body>
    </html>
    """

@app.route('/file', methods=['DELETE'])
def handle_delete():
    file_name = request.args.get('file')
    if file_name == 'prueba.html':
        return jsonify({"title": "ERROR 403", "message": f"El fichero {file_name} esta protegido y el servidor deniega la acción solicitada"}), 403
    try:
        os.remove(file_name)
        return jsonify({"title": "Archivo eliminado", "message": f"El fichero {file_name} ha sido eliminado satisfactoriamente"}), 202
    except FileNotFoundError:
        return jsonify({"title": "ERROR 404", "message": f"El fichero {file_name} no pudo ser eliminado, es probable que no exista en el servidor"}), 404

@app.route('/file', methods=['PUT'])
def handle_put():
    file_name = request.args.get('file')
    file_exists = os.path.exists(file_name)
    with open(file_name, 'w') as f:
        f.write(request.data.decode())
    if file_exists:
        return f"""
        <?xml version="1.0" encoding="UTF-8"?>
        <response>
            <title>SERVIDOR WEB - Actualización</title>
            <message>El fichero {file_name} fue modificado exitosamente</message>
        </response>
        """, 202
    else:
        return f"""
        <?xml version="1.0" encoding="UTF-8"?>
        <response>
            <title>SERVIDOR WEB - Creación</title>
            <message>El fichero {file_name} fue creado exitosamente</message>
        </response>
        """, 201

@app.route('/file', methods=['HEAD'])
def handle_head():
    file_name = request.headers.get('File-Name')
    if not file_name:
        return '', 400, {"Content-Type": "text/plain", "Date": datetime.now().strftime("%a, %d %b %Y %H:%M:%S GMT")}

    if os.path.exists(file_name):
        # Obtener la extensión del archivo y su tipo MIME
        extension = file_name.split('.')[-1]
        mime_type = 'text/plain'

        if extension == 'html':
            mime_type = 'text/html'
        elif extension == 'css':
            mime_type = 'text/css'
        elif extension == 'js':
            mime_type = 'application/javascript'
        elif extension == 'jpg' or extension == 'jpeg':
            mime_type = 'image/jpeg'
        elif extension == 'png':
            mime_type = 'image/png'
        elif extension == 'gif':
            mime_type = 'image/gif'
        elif extension == 'pdf':
            mime_type = 'application/pdf'
        elif extension == 'mp4':
            mime_type = 'video/mp4'
        elif extension == 'mp3':
            mime_type = 'audio/mpeg'

        file_size = os.path.getsize(file_name)
        file_size = convertBytestoString(file_size)

        headers = {
            "Content-Type": mime_type,
            "Size": file_size,
        }
        return '', 200, headers
    else:
        return '', 404, {"Date": datetime.now().strftime("%a, %d %b %Y %H:%M:%S GMT")}

def convertBytestoString(data):
    if data < 1024:
        return f"{data} Bytes"
    elif data < 1024**2:
        return f"{data / 1024} KB"
    elif data < 1024**3:
        return f"{data / 1024**2} MB"
    elif data < 1024**4:
        return f"{data / 1024**3} GB"
    else:
        return f"{data / 1024**4} TB"

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=8000)
