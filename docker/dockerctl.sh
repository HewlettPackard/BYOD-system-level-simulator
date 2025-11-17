#! /usr/bin/env bash

set -e

function usage() {
    echo "Usage: ./dockerctl.sh [OPTION]"
    echo "  -b, --build <TAG> <DOCKERFILE> <PORT>          Builds and tags a docker image"
    echo "  -r, --run <IMAGE> <CONTAINER> <PATH> <PORT>    Starts container"
    echo "  -s, --launch <IMAGE> <CONTAINER> <PATH> <PORT> Gets a shell on container"
    echo "  -rm, --remove <CONTAINER>                      Stops and removes container"
    echo "  -h, --help                                     Print this message"
    exit 0
}

if [ $# -eq 0 ]
then
    usage
    exit
fi

ARCH="$(uname -m)"
case $ARCH in 
    x86_64) 
        rustArch='x86_64-unknown-linux-gnu'
        rustupSha256='e6599a1c7be58a2d8eaca66a80e0dc006d87bbcf780a58b7343d6e14c1605cb2'
        ;; 
    aarch64) 
        rustArch='aarch64-unknown-linux-gnu'
        rustupSha256='a97c8f56d7462908695348dd8c71ea6740c138ce303715793a690503a94fc9a9'
        ;; 
    *) 
        echo >&2 "unsupported architecture: $ARCH"
        exit 1 
        ;; 
esac

# uncomment following line and adapt if behind a proxy
# PROXY_URL=http://web-proxy.corp.hpecorp.net:8080/
JUPYTER_PORT=8888

while test $# -gt 0
do
    case "$1" in
        -b|--build)
            docker build -t "$2" \
                   --build-arg=PORTS="$4" \
                   --build-arg=http_proxy="${PROXY_URL}" \
                   --build-arg=https_proxy="${PROXY_URL}" \
                   --build-arg=RUST_ARCH="${rustArch}"\
                   -f "$3" .
            shift
            shift
            shift
            ;;
        -r|--run)
            docker run --rm -p "$5":"${JUPYTER_PORT}" \
		 --user root \
                 --ipc=host \
		 --privileged \
                 --name "$3" \
                 -v "$4":/home/"${USER}"/byod \
		 -e NB_USER="${USER}" \
		 -e NB_UID="$(id -u)" \
		 -e NB_GID="$(id -g)"  \
		 -e CHOWN_HOME=yes \
		 -e CHOWN_EXTRA="/home/${USER}/byod" \
                 "$2"
            shift
            shift
            shift
            shift
            ;;
        -s|--shell)
            docker run --rm -it -p "$5":"${JUPYTER_PORT}" \
		 --user root \
         --ipc=host \
		 --privileged \
         --name "$3" \
         -v "$4":/home/"${USER}"/byod \
		 -e NB_USER="${USER}" \
		 -e NB_UID="$(id -u)" \
		 -e NB_GID="$(id -g)"  \
		 -e CHOWN_HOME=yes \
		 -e CHOWN_EXTRA="/home/${USER}/byod" \
		 -e CHOWN_EXTRA_OPTS='-R' \
                 "$2" \
                 /bin/bash
            shift
            shift
            shift
            shift
            ;;
        -rm|--remove)
            docker container rm -f "$2"
            shift
            ;;
        --help|-h|*)
            usage
            ;;
    esac
    shift
done

exit 0
