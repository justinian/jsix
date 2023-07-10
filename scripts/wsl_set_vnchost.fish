set -gx VNCHOST (ip -j route list default | jq -r '.[0].gateway'):5500
