FROM node:22-alpine AS build
WORKDIR /app
COPY apps/pubsub-3d-webapp/package.json ./package.json
RUN npm install
COPY apps/pubsub-3d-webapp/ ./
ARG VITE_RELAY_URL=http://localhost:8080
ARG VITE_STREAM_PROTOCOL=ws
ENV VITE_RELAY_URL=$VITE_RELAY_URL
ENV VITE_STREAM_PROTOCOL=$VITE_STREAM_PROTOCOL
RUN npm run build

FROM nginx:1.27-alpine
COPY apps/pubsub-3d-webapp/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /app/dist /usr/share/nginx/html
ENV VITE_RELAY_URL=""
ENV VITE_STREAM_PROTOCOL="ws"
EXPOSE 8080
CMD ["/bin/sh", "-c", "printf 'window.__APP_CONFIG__ = { relayUrl: \"%s\", streamProtocol: \"%s\" };\\n' \"$VITE_RELAY_URL\" \"$VITE_STREAM_PROTOCOL\" > /usr/share/nginx/html/config.js && nginx -g 'daemon off;'"]
