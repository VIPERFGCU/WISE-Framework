# ---------- Build stage ----------
FROM node:22-alpine AS build
WORKDIR /app

# Install deps
COPY package*.json ./
# Use npm ci if you have a package-lock.json; fallback to npm install
RUN if [ -f package-lock.json ]; then npm ci; else npm install; fi

# Copy source and build
COPY . .
RUN npm run build

# ---------- Runtime stage ----------
FROM nginx:stable-alpine
# Nginx config with /api proxy to FastAPI service "api"
COPY nginx.conf /etc/nginx/conf.d/default.conf
# Static build output from Vite
COPY --from=build /app/dist /usr/share/nginx/html
EXPOSE 80

