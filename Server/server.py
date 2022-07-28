from flask import Flask, json
import httplib, urllib
import secrets as Secrets

api = Flask(__name__)

@api.route('/floating', methods=['GET'])
def get_floating():
  conn = httplib.HTTPSConnection("pushsafer.com:443")
  conn.request("POST", "/api",
    urllib.urlencode({
      "k": Secrets.apiKey,                
      "m": "Alarma Deposito",                 
      "t": "El deposito esta lleno",                     
      "i": "1",                         # Icon number 1-98
      "s": "8",                         # Sound number 0-28
      "v": "2",                         # Vibration number 0-3
      "p": "",                          # Picture Data URL with Base64-encoded string
    }), { "Content-type": "application/x-www-form-urlencoded" })
  response = conn.getresponse()
  print response.status, response.reason
  data = response.read()
  print data
  return data

if __name__ == '__main__':
    api.run(host='0.0.0.0')