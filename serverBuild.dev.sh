IMAGE_NAME="sysperform.dev"
CONTAINER_NAME="sysperform.dev"
PORT=8443
HOST="localhost"
DOCKERFILE="./Dockerfile.dev"


# Step 1: Build the Docker image
echo "🔨 Building Docker image..."
docker build -t $IMAGE_NAME -f Dockerfile.dev .

# Step 2: Stop and remove any existing container with the same name
if [ "$(docker ps -aq -f name=$CONTAINER_NAME)" ]; then
    echo "🧹 Removing existing container..."
    docker rm -f $CONTAINER_NAME
fi

echo "🚀 Starting new container..."
docker run -d --name $CONTAINER_NAME \
  -v $(pwd)/app/app/data:/app/app/data \
  -p 8443:8443 \
  $IMAGE_NAME


echo "✅ Server is running at https://$HOST:$PORT"

echo "🛠️  Opening Log files...."

docker logs -f $CONTAINER_NAME
