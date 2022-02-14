ROOT_DIR="$(pwd)"

docker build -t dramsim3-docker .
docker run --rm -it --name="dramsim3" -v "${ROOT_DIR}/":/DRAMSIM3 -w="/DRAMSIM3" dramsim3-docker