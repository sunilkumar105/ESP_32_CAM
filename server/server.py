from flask import Flask, request
import logging
import os

app = Flask(__name__)

@app.route('/', methods=['GET'])
def home():
    return "Hello world!"

@app.route('/upload', methods=["GET", 'POST'])
def upload_image():
    if request.method == "POST": #if we make a post request to the endpoint, look for the image in the request body
        image_raw_bytes = request.get_data()  #get the whole body

        save_location = (os.path.join(app.root_path, "C:/Users/sunil/Desktop/FLASJ_SERVER/test.jpg")) #save to the same folder as the flask app live in 

        f = open(save_location, 'wb') # wb for write byte data in the file instead of string
        f.write(image_raw_bytes) #write the bytes from the request body to the file
        f.close()

        print("Image saved")

        return "image saved"    

if (__name__ == '__main__'):
    app.run(host='192.168.1.10', port=5000, debug=True, threaded=False)