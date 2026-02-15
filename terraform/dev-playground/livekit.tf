# LiveKit namespace
resource "kubernetes_namespace" "livekit" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name = "livekit"
    labels = {
      name = "livekit"
    }
  }

  depends_on = [google_container_node_pool.livekit_nodes[0]]
}

# Generate random secret for LiveKit API
resource "random_password" "livekit_secret" {
  count = var.enable_livekit ? 1 : 0

  length  = 32
  special = false
}

# Install LiveKit Server via Helm
resource "helm_release" "livekit_server" {
  count = var.enable_livekit ? 1 : 0

  name       = "livekit-server"
  repository = "https://helm.livekit.io"
  chart      = "livekit-server"
  version    = var.livekit_version
  namespace  = kubernetes_namespace.livekit[0].metadata[0].name

  values = [
    templatefile("${path.module}/livekit-values.yaml", {
      livekit_version    = var.livekit_version
      redis_host         = "redis.redis.svc.cluster.local"
      redis_port         = "6379"
      domain             = var.livekit_domain
      livekit_api_key    = "devkey"
      livekit_api_secret = random_password.livekit_secret[0].result
    })
  ]

  depends_on = [
    kubernetes_service.redis[0],
    kubectl_manifest.livekit_certificate[0],
    helm_release.nginx_ingress_certbot[0]
  ]
}

# Get the NGINX Ingress LoadBalancer IP
data "kubernetes_service" "nginx_ingress_lb" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name      = "nginx-ingress-ingress-nginx-controller"
    namespace = "ingress-nginx"
  }

  depends_on = [helm_release.nginx_ingress_certbot[0]]
}

# Get the TURN LoadBalancer IP
data "kubernetes_service" "turn_lb" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name      = "livekit-server-turn"
    namespace = kubernetes_namespace.livekit[0].metadata[0].name
  }

  depends_on = [helm_release.livekit_server[0]]
}
