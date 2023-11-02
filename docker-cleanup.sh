set -e
docker ps --filter "status=exited" --no-trunc -aq       | \
    xargs --no-run-if-empty docker rm
docker images --no-trunc | grep umit | awk '{print $3}' | \
    xargs --no-run-if-empty docker rmi -f

