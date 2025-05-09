IMAGE_NAME="sysperform.dev"
CONTAINER_NAME="sysperform.dev"
PORT=8443
HOST="localhost"
DOCKERFILE="./Dockerfile"


# Step 1: Build the Docker image
echo "🔨 Building Docker image..."
docker build -t $IMAGE_NAME -f Dockerfile .

# Step 2: Stop and remove any existing container with the same name
if [ "$(docker ps -aq -f name=$CONTAINER_NAME)" ]; then
    echo "🧹 Removing existing container..."
    docker rm -f $CONTAINER_NAME
fi

echo "🚀 Starting new container..."
docker run -d \
  --name $CONTAINER_NAME \
  -v $(pwd)/app/data:/app/data \
  -p $PORT:$PORT \
  $IMAGE_NAME

echo "✅ Server is running at http://$HOST:$PORT"

echo "Opening logs...."
docker logs -f $CONTAINER_NAME
