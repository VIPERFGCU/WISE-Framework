# WISENET Project Management

.PHONY: build up down clean restart

# Perform a clean build of all services, ensuring no stale caches
build:
	docker compose build --no-cache

# Start the services in the background
up:
	docker compose up -d

# Stop and remove containers
down:
	docker compose down

# The "Nuclear Option": Clean build and restart everything
restart:
	docker compose down
	docker compose build --no-cache frontend
	docker compose up -d

# Check logs for the API and Frontend
logs:
	docker compose logs -f api frontend