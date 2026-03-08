import os
import json
from flask import Flask, request, Response

ttml_index_file = os.path.expanduser(os.environ.get("NIL_SERVER_TTML_JSON_AND_WHY_DID_I_USE_PYTHON", "~/MyCoolTTMLs/ttml.json"))

app = Flask(__name__)

TTML_INDEX = {}
try:
    with open(ttml_index_file, "r") as f:
        entries = json.load(f)
        for entry in entries:
            key = (entry.get("songname"), entry.get("artist"))
            path = os.path.expanduser(entry.get("ttmlpath", ""))
            if key[0] and key[1] and os.path.isfile(path):
                try:
                    with open(path, "r") as ttml_file:
                        TTML_INDEX[key] = ttml_file.read()
                except Exception:
                    continue
except Exception:
    print(f"ooo fuck {ttml_index_file}")

@app.route("/lyrics")
def lyrics():
    song = request.args.get("title", "").strip()
    artist = request.args.get("artist", "").strip()
    ttml = TTML_INDEX.get((song, artist))
    if ttml:
        return Response(ttml, mimetype="application/ttml+xml")
    else:
        return Response("no-lyrics-found", status=404, mimetype="text/plain")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=6741, debug=True)
