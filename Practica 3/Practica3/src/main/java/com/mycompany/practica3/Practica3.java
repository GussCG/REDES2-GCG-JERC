package com.mycompany.practica3;

import java.io.*;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.net.*;

public class Practica3 {

    public static final int PUERTO = 8000; //Puerto en el que escucha el servidor
    public static final int TAM_POOL = 100; //Tamaño del pool de Hilos
    ServerSocket ss;

    class Manejador extends Thread //Clase manejador  para poder crear varios hilos
    {

        protected Socket socket; //Socket de cada cliente
        protected PrintWriter pw;
        protected BufferedOutputStream bos;
        protected BufferedReader br;
        DataOutputStream dos;
        DataInputStream dis;
        protected String FileName; //Nombre del archivo que se va a manejar

        public Manejador(Socket _socket) throws Exception //Constructor de la clase manejador
        {
            this.socket = _socket;
        }

        @Override
        public void run() {
            try {
                dos = new DataOutputStream(socket.getOutputStream());
                dis = new DataInputStream(socket.getInputStream());
                byte[] b = new byte[1024];
                int t = dis.read(b); //Leemos los datos que llegan y los guardamos en 'b'
                String peticion = new String(b, 0, t); //Convertimos esos datos a String
                if (peticion == null) //Si la petición es null, enviamos un mensaje de error al cliente
                {
                    StringBuffer msj = new StringBuffer(); //Creamos un StringBuffer para construir el mensaje
                    msj.append("<html><head><title>Servidor WEB\n");
                    msj.append("</title><body bgcolor=\"#AACCFF\"<br>Linea Vacia</br>\n");
                    msj.append("</body></html>\n");
                    dos.write(msj.toString().getBytes()); //Escribimos el mensaje en el DataOutputStream
                    dos.flush();
                    socket.close(); //Se Cierra el socket
                    return;
                }
                System.out.println("Cliente Conectado desde: " + socket.getInetAddress());
                System.out.println("Por el puerto: " + socket.getPort());
                StringTokenizer token = new StringTokenizer(peticion, "\n"); //Separamos la petición por líneas
                String line = token.nextToken(); //Obtenemos la primera línea de la petición
                System.out.println("METODO: " + line); //Imprimimos la primera línea

                //Metodo GET 
                if (line.indexOf("?") == -1 && line.toUpperCase().startsWith("GET")) //Si la línea no contiene "?" y empieza con "GET"
                {
                    getArch(line);  //Obtener el archivo solicitado
                    if (FileName.compareTo("") == 0) //Si el nombre del archivo es vacío
                    {
                        SendA("index.htm", dos);  //Enviar el archivo "index.htm"
                    } else {
                        SendA(FileName, dos); //Enviar el archivo solicitado
                    }
                } else if (line.toUpperCase().startsWith("GET")) //Si la línea empieza con "GET"
                {
                    System.out.println("METODO GET");
                    StringTokenizer tokens = new StringTokenizer(line, "?");
                    String req_a = tokens.nextToken();
                    String req = tokens.nextToken();
                    String parametros = req.substring(0, req.indexOf(" ")) + "\n";  //Extrae los parámetros de la solicitud GET
                    System.out.println("Parametros Obtenidos: " + parametros);
                    StringBuffer respuesta = new StringBuffer();  //Crea una respuesta al cliente                                              
                    respuesta.append("HTTP/1.0 200 Okay \n"); //Código de estado HTTP 200, que indica éxito
                    String fecha = "Date: " + new Date() + " \n"; //Fecha actual
                    respuesta.append(fecha);
                    String tipo_mime = "Content-Type: text/html \n";  //Tipo de contenido de la respuesta
                    respuesta.append(tipo_mime);

                    respuesta.append("Cache-Control: no-cache\n");
                    String server = "Server: Practica3\n";
                    respuesta.append(server);
                    respuesta.append("Connection: close\n\n");
                    respuesta.append("<html><head><title>SERVIDOR WEB</title></head>\n");
                    respuesta.append("<body bgcolor=\"#F99ED9\"><center><h1><br>Parametros Obtenidos por el Metodo GET</br></h1><h3><b>\n");
                    respuesta.append(parametros);
                    respuesta.append("</b></h3>\n");
                    respuesta.append("</center></body></html>\n\n");
                    System.out.println("HEADERS\n" + respuesta);
                    dos.write(respuesta.toString().getBytes()); //Envia la respuesta al cliente
                    dos.flush();
                    dos.close(); //Cierra  el flujo de salida
                    socket.close(); //Cierra el socket
                } //Metodo POST
                //Similar a la sección de código anterior, pero para solicitudes POST
                //Aquí se extraen los parámetros del cuerpo de la solicitud POST en lugar de la URL
                else if (line.toUpperCase().startsWith("POST")) { //Si la línea empieza con "POST"
                    System.out.println("PETICION POST");
                    /* Crea un StringTokenizer que va a dividir la petición "\n" (con saltos de linea) */
                    StringTokenizer stokens = new StringTokenizer(peticion, "\n");
                    String _line_ = stokens.nextToken();  // Recoge el primer token de la petición

                    /* Mientras que la línea no empiece con "Nombre" y todavía queden tokens en stokens,   sigue cogiendo el siguiente token y asignándolo en la mimsa linea */
                    while (!_line_.startsWith("Nombre") && stokens.hasMoreElements()) {
                        _line_ = stokens.nextToken();
                    }
                    String parametros = _line_; // Asigna los parámetros de la línea
                    System.out.print("Parametros Obtenidos:" + parametros);  // Imprime los parámetros
                    // Crea un StringBuffer donde se almacenará la respuesta
                    StringBuffer respuesta = new StringBuffer();
                    respuesta.append("HTTP/1.0 201 Okay \n");
                    String fecha = "Date: " + new Date() + " \n";  // Obtiene la fecha actual y la agrega a la respuesta
                    respuesta.append(fecha);
                    // Define el tipo de contenido (MIME type) de la respuesta
                    String tipo_mime = "Content-Type: text/html\n";
                    respuesta.append(tipo_mime);
                    respuesta.append("Cache-Control: no-cache\n");
                    String server = "Server: Practica3\n";
                    respuesta.append(server);
                    respuesta.append("Connection: close\n\n");
                    // Comienza a construir el cuerpo de la respuesta en formato HTML
                    respuesta.append("<html><head><title>SERVIDOR WEB</title></head>\n");
                    respuesta.append("<body bgcolor=\"#DABEFA\"><center><h1><br>Parametros Obtenidos mediante el Metodo POST</br></h1><h3><b>\n");
                    respuesta.append(parametros);  // Agrega los parámetros obtenidos al cuerpo del mensaje HTML
                    respuesta.append("</b></h3>\n");
                    respuesta.append("</center></body></html>\n\n");  // Finaliza el cuerpo del mensaje HTML
                    System.out.println("\n\nHEADERS\n\n" + respuesta);
                    // Envía la respuesta al cliente
                    dos.write(respuesta.toString().getBytes());
                    dos.flush();
                    // Cierra el stream de salida y el socket
                    dos.close();
                    socket.close();
                } //Metodo DELETE
                //Se extrae el recurso solicitado de la URL y se intenta eliminar
                //Se envía una respuesta al cliente indicando si la eliminación fue exitosa o no
                else if (line.toUpperCase().startsWith("DELETE")) { //Si la línea empieza con "DELETE"
                    StringTokenizer tokens = new StringTokenizer(line, " ");
                    String req_a = tokens.nextToken();
                    String req = tokens.nextToken();
                    Path path = Paths.get("");
                    String directoryName = path.toAbsolutePath().toString();
                    System.out.println(directoryName + req); // Muestra la ruta del archivo
                    File fichero = new File(directoryName + req);
                    String response = ""; // Almacena la respuesta
                    String title = ""; // Almacena el título
                    String statusCode = "";

                    // Comprueba si el nombre del fichero es "index2.htm"
                    if (fichero.getName().equals("prueba.html")) {
                        title += "ERROR 403";
                        response = "El fichero " + fichero.getName() + " esta protegido y el servidor deniega la acción solicitada, página web o servicio.";
                        statusCode = "403 Forbidden";
                    } // Intenta eliminar el fichero y comprueba si fue exitoso
                    else if (fichero.delete()) {
                        response = "El fichero " + fichero.getName() + " ha sido eliminado satisfactoriamente";
                        title += "Archivo eliminado";
                        statusCode = "202 Okay";
                    } // En caso de no poder eliminar el fichero
                    else {
                        response = "El fichero " + fichero.getName() + " no pudo ser eliminado, es probable que no exista en el servidor";
                        title += "ERROR 404";
                        statusCode = "404 Not Found";
                    }
                    System.out.println(response); // Muestra la respuesta
                    StringBuffer respuesta = new StringBuffer();
                    respuesta.append("HTTP/1.0 " + statusCode + " \n");
                    String fecha = "Date: " + new Date() + " \n";
                    respuesta.append(fecha);
                    String tipo_mime = "Content-Type: application/json\n";
                    respuesta.append(tipo_mime);
                    respuesta.append("Cache-Control: no-cache\n");
                    String server = "Server: Practica3\n";
                    respuesta.append(server);
                    respuesta.append("Connection: close\n\n");
                    respuesta.append("{\"title\":\"" + title + "\",\"message\":\"" + response + "\"}");
                    System.out.println("HEADERS\n\n" + respuesta);
                    dos.write(respuesta.toString().getBytes());
                    dos.flush();
                    dos.close();
                    socket.close();
                } //Metodo PUT
                //Se extrae el recurso solicitado de la URL y se intenta crear o reemplazar
                //Se envía una respuesta al cliente indicando si la operación fue exitosa o no
                else if (line.toUpperCase().startsWith("PUT")) {
                    String xmlData;
                    StringTokenizer tokens = new StringTokenizer(line, " ");
                    String req_a = tokens.nextToken();
                    String req = tokens.nextToken();
                    Path path = Paths.get("");
                    String directoryName = path.toAbsolutePath().toString();
                    System.out.print("Ruta del archivo creado o modificado:");
                    System.out.println(directoryName + req);// Muestra la ruta del archivo
                    File fichero = new File(directoryName + req);

                    // Mensaje de contenido
                    String contenido;

                    // Verificar si el archivo ya existe
                    boolean fileExists = fichero.exists();
                    if (fileExists) {
                        contenido = "El archivo ya existe. El contenido se ha modificado.";
                    } else {
                        contenido = "El archivo ha sido creado correctamente.";
                    }

                    StringBuffer respuesta = new StringBuffer();  // Crea un StringBuffer para la respuesta
                    respuesta.append("HTTP/1.0 202 Okay \n");
                    String fecha = "Date: " + new Date() + " \n";
                    respuesta.append(fecha);
                    String tipo_mime = "Content-Type: application/xml \n";
                    respuesta.append(tipo_mime);
                    respuesta.append("Cache-Control: no-cache\n");
                    String server = "Server: Practica3\n";
                    respuesta.append(server);
                    respuesta.append("Connection: close\n\n");
                    if (fileExists) {
                        respuesta.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
                        respuesta.append("<response>\n");
                        respuesta.append("<title>SERVIDOR WEB - Actualización</title>\n");
                        respuesta.append("<message>El fichero " + fichero.getName() + " fue modificado exitosamente</message>\n");
                        respuesta.append("</response>");
                    } else {
                        respuesta.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
                        respuesta.append("<response>\n");
                        respuesta.append("<title>SERVIDOR WEB - Creación</title>\n");
                        respuesta.append("<message>El fichero " + fichero.getName() + " fue creado exitosamente</message>\n");
                        respuesta.append("</response>");
                    }
                    System.out.println("\nHEADERS\n" + respuesta); // Muestra la respuesta del Servidor
                    dos.write(respuesta.toString().getBytes());
                    dos.flush();
                    dos.close();
                    socket.close();
                } 
                //Metodo HEAD
                //Se envía una respuesta al cliente con los encabezados que se enviarían en una solicitud GET equivalente
                else if (line.toUpperCase().startsWith("HEAD")) { // Si la línea empieza con "HEAD"
                    File imagen = new File("EscudoESCOM.png");
                    int contentLength = (int) imagen.length();
                    StringBuffer respuesta = new StringBuffer();
                    respuesta.append("HTTP/1.0 200 Okay\n");
                    String fecha = "Date: " + new Date() + "\n";
                    respuesta.append(fecha);
                    String tipo_mime = "Content-Type: image/jpeg\n";
                    respuesta.append(tipo_mime);
                    String contentLengthHeader = "Content-Length: " + contentLength + "\n";
                    respuesta.append(contentLengthHeader);
                    respuesta.append("Cache-Control: no-cache\n");
                    String server = "Server: Practica3\n";
                    respuesta.append(server);
                    respuesta.append("Connection: close\n\n");
                    System.out.println("HEADERS\n" + respuesta);
                    dos.write(respuesta.toString().getBytes());
                    dos.flush();
                    FileInputStream fis = new FileInputStream(imagen);
                    byte[] buffer = new byte[1024];
                    int bytesRead;
                    while ((bytesRead = fis.read(buffer)) != -1) {
                        dos.write(buffer, 0, bytesRead);
                    }
                    dos.flush();
                    fis.close();
                    dos.close();
                    socket.close();
                } else { // Envía un código de estado HTTP 501, que indica que el método solicitado no es soportado por el servidor
                    dos.write("HTTP/1.0 501 Not Implemented\r\n".getBytes());
                    dos.flush();
                    dos.close(); // Cierra el flujo de salida
                    socket.close(); // Cierra el socket
                }
            } catch (Exception e) //Manejo de Excepciones
            {
                e.printStackTrace();
            }
        }

        public String getMimeType(String fileName) {
            String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
            switch (ext) {
                case "html":
                case "htm":
                    return "text/html";
                case "jpg":
                case "jpeg":
                    return "image/jpeg";
                case "png":
                    return "image/png";
                case "css":
                    return "text/css";
                case "js":
                    return "application/javascript";
                case "pdf":
                    return "document/pdf";
                case "doc":
                    return "document/word";
                case "xml":
                    return "application/xml";
                case "json":
                    return "application/json";
                default:
                    return "application/octet-stream";
            }
        }

        // Método que retorna la fecha y hora actual
        public String showDate() {
            String timeStamp = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss").format(Calendar.getInstance().getTime());
            return "Time Now: " + timeStamp;
        }

        // Método para obtener el nombre del archivo solicitado.
        public void getArch(String line) {
            int i;
            int f;
            // Este método se utiliza cuando se recibe una petición GET, para identificar qué recurso se ha solicitado.
            if (line.toUpperCase().startsWith("GET")) {
                i = line.indexOf("/");
                f = line.indexOf(" ", i);
                FileName = line.substring(i + 1, f);
            } // Si la petición no comienza con "GET", se deja el nombre del archivo como una cadena vacía.
            else {
                FileName = "";
            }
        }

        // Método para enviar un archivo al cliente.
        // Recibe el nombre del archivo, el socket del cliente y un flujo de datos de salida como parámetros.
        public void SendA(String fileName, Socket sc, DataOutputStream dos) {
            int fSize = 0;
            // Buffer para almacenar los datos del archivo.
            byte[] buffer = new byte[4096];
            try {
                DataInputStream dis1 = new DataInputStream(new FileInputStream(fileName));
                int x = 0;
                File ff = new File("fileName");
                long tam, cont = 0;
                tam = ff.length();
                // Lee el archivo y lo envía al cliente en bloques de 4096 bytes.
                while (cont < tam) {
                    x = dis1.read(buffer);
                    dos.write(buffer, 0, x);
                    cont = cont + x;
                    dos.flush();
                }
                // Cierra el archivo y el flujo de datos de salida
                dis.close();
                dos.close();
                // Manejo de excepciones
            } catch (FileNotFoundException e) {
            } catch (IOException e) {
            }
        }

        // Método para enviar un archivo al cliente con una cabecera HTTP adecuada.
        // Recibe como argumentos el nombre del archivo y un flujo de datos de salida.
        public void SendA(String arg, DataOutputStream dos1) {
            try {
                int b_leidos = 0;
                DataInputStream dis2 = new DataInputStream(new FileInputStream(arg));
                // Buffer para almacenar los datos del archivo.
                byte[] buf = new byte[1024];
                // Abre el archivo para leerlo.
                int x = 0;
                File ff = new File(arg);
                long tam_archivo = ff.length(), cont = 0;
                // Cabecera HTTP
                String msj = "";
                msj = msj + "HTTP/1.0 200 ok\n";
                msj = msj + "Server: Servidor \n";
                msj = msj + "Date: " + new Date() + " \n";
                msj = msj + "Content-Type: " + getMimeType(arg) + " \n";
                msj = msj + "Content-Length: " + tam_archivo + " \n";
                msj = msj + "\n";
                // Envío de la cabecera HTTP
                dos1.write(msj.getBytes());
                dos1.flush();
                // LeE el archivo y envíA al cliente en bloques de 1024 bytes
                while (cont < tam_archivo) {
                    x = dis2.read(buf);
                    dos1.write(buf, 0, x);
                    cont = cont + x;
                    dos1.flush();
                }
                // Cierra los flujos de entrada y salida de datos
                dis2.close();
                dos1.close();
            } // Imprime cualquier error que pueda surgir durante la lectura/envío del archivo.
            catch (Exception e) {
                System.out.println(e.getMessage());
            }
        }
    }
    // Constructor de la clase ServidorHTTP.

    public Practica3() {
        try {
            // Crea un pool de threads con un tamaño de 100
            ExecutorService pool = Executors.newFixedThreadPool(TAM_POOL);
            System.out.println("Iniciando Servidor...");
            System.out.println("Pool de Conexiones: " + TAM_POOL);
            // Inicializa el socket del servidor en un puerto específico.
            this.ss = new ServerSocket(PUERTO);
            System.out.println("Servidor iniciado: OK");
            // Imprime la URL del servidor.
            URL myURL = new URL("http://127.0.0.1:" + PUERTO);
            System.out.println("Servidor iniciado en: " + myURL);
            System.out.println("Esperando al Cliente...");

            // Bucle infinito para aceptar y manejar múltiples conexiones de clientes.
            for (;;) {
                Socket accept = ss.accept();
                Manejador manejador = new Manejador(accept);
                // Agrega el manejador al pool de threads.
                pool.execute(manejador);
            }
        } // Imprime cualquier error que pueda surgir durante la inicialización del servidor.
        catch (Exception e) {
            e.printStackTrace();
        }

    } // Método main, donde se inicia el servidor.

    public static void main(String[] args) throws Exception {
        Practica3 sWEB = new Practica3();
    }

}
