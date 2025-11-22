# LiveKit namespace
resource "kubernetes_namespace" "livekit" {
  metadata {
    name = "livekit"
    labels = {
      name = "livekit"
    }
  }

  depends_on = [google_container_node_pool.livekit_nodes]
}

# Generate random secret for LiveKit API
resource "random_password" "livekit_secret" {
  length  = 32
  special = false
}

# Install LiveKit Server via Helm
resource "helm_release" "livekit_server" {
  name       = "livekit-server"
  repository = "https://helm.livekit.io"
  chart      = "livekit-server"
  version    = var.livekit_version
  namespace  = kubernetes_namespace.livekit.metadata[0].name

  values = [
    templatefile("${path.module}/livekit-values.yaml", {
      livekit_version    = var.livekit_version
      redis_host         = "redis.redis.svc.cluster.local"
      redis_port         = "6379"
      domain             = var.livekit_domain
      livekit_api_key    = "devkey"
      livekit_api_secret = random_password.livekit_secret.result
    })
  ]

  depends_on = [
    kubernetes_service.redis,
    kubectl_manifest.livekit_certificate,
    helm_release.nginx_ingress_certbot
  ]
}

# Get the NGINX Ingress LoadBalancer IP
data "kubernetes_service" "nginx_ingress_lb" {
  metadata {
    name      = "nginx-ingress-ingress-nginx-controller"
    namespace = "ingress-nginx"
  }

  depends_on = [helm_release.nginx_ingress_certbot]
}

# Get the TURN LoadBalancer IP
data "kubernetes_service" "turn_lb" {
  metadata {
    name      = "livekit-server-turn"
    namespace = kubernetes_namespace.livekit.metadata[0].name
  }

  depends_on = [helm_release.livekit_server]
}
