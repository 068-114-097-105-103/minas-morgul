IMAGE_NAME="sysperform.proto"
PORT=443
HOST="sysperform.net"
DOCKERFILE="./Dockerfile"


# Step 1: Build the Docker image
echo "🔨 Building Docker image..."
docker build -t $IMAGE_NAME -f Dockerfile.dev .
