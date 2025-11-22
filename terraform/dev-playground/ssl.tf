# cert-manager namespace
resource "kubernetes_namespace" "cert_manager" {
  metadata {
    name = "cert-manager"
  }

  depends_on = [google_container_node_pool.livekit_nodes]
}

# Install cert-manager via Helm
resource "helm_release" "cert_manager" {
  name       = "cert-manager"
  repository = "https://charts.jetstack.io"
  chart      = "cert-manager"
  version    = "v1.13.3"
  namespace  = kubernetes_namespace.cert_manager.metadata[0].name

  set {
    name  = "installCRDs"
    value = "true"
  }

  set {
    name  = "global.leaderElection.namespace"
    value = kubernetes_namespace.cert_manager.metadata[0].name
  }

  depends_on = [google_container_node_pool.livekit_nodes]
}

# ClusterIssuer for Let's Encrypt production (using DNS-01 challenge)
resource "kubectl_manifest" "letsencrypt_prod" {
  yaml_body = <<-YAML
    apiVersion: cert-manager.io/v1
    kind: ClusterIssuer
    metadata:
      name: letsencrypt-prod
    spec:
      acme:
        server: https://acme-v02.api.letsencrypt.org/directory
        email: admin@haffi.dev
        privateKeySecretRef:
          name: letsencrypt-prod-private-key
        solvers:
        - http01:
            ingress:
              class: nginx
  YAML

  depends_on = [helm_release.cert_manager]
}

# Certificate for LiveKit domain
resource "kubectl_manifest" "livekit_certificate" {
  yaml_body = <<-YAML
    apiVersion: cert-manager.io/v1
    kind: Certificate
    metadata:
      name: livekit-tls
      namespace: livekit
    spec:
      secretName: livekit-tls-secret
      issuerRef:
        name: letsencrypt-prod
        kind: ClusterIssuer
      dnsNames:
      - ${var.livekit_domain}
      privateKey:
        rotationPolicy: Always
  YAML

  depends_on = [
    kubectl_manifest.letsencrypt_prod,
    kubernetes_namespace.livekit
  ]
}

# ClusterIssuer for Let's Encrypt staging (for testing)
resource "kubectl_manifest" "letsencrypt_staging" {
  yaml_body = <<-YAML
    apiVersion: cert-manager.io/v1
    kind: ClusterIssuer
    metadata:
      name: letsencrypt-staging
    spec:
      acme:
        server: https://acme-staging-v02.api.letsencrypt.org/directory
        email: admin@haffi.dev
        privateKeySecretRef:
          name: letsencrypt-staging-private-key
        solvers:
        - http01:
            ingress:
              class: nginx
  YAML

  depends_on = [helm_release.cert_manager]
}

# Temporary NGINX Ingress for HTTP-01 challenge
resource "helm_release" "nginx_ingress_certbot" {
  name             = "nginx-ingress"
  repository       = "https://kubernetes.github.io/ingress-nginx"
  chart            = "ingress-nginx"
  version          = "4.8.3"
  namespace        = "ingress-nginx"
  create_namespace = true

  set {
    name  = "controller.service.type"
    value = "LoadBalancer"
  }

  set {
    name  = "controller.publishService.enabled"
    value = "true"
  }

  depends_on = [google_container_node_pool.livekit_nodes]
}

# Ingress for LiveKit with SSL/TLS
resource "kubectl_manifest" "livekit_ingress_http01" {
  yaml_body = <<-YAML
    apiVersion: networking.k8s.io/v1
    kind: Ingress
    metadata:
      name: livekit-http01
      namespace: livekit
      annotations:
        kubernetes.io/ingress.class: nginx
        cert-manager.io/cluster-issuer: letsencrypt-prod
        nginx.ingress.kubernetes.io/ssl-redirect: "true"
    spec:
      tls:
      - hosts:
        - ${var.livekit_domain}
        secretName: livekit-tls-secret
      rules:
      - host: ${var.livekit_domain}
        http:
          paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: livekit-server
                port:
                  number: 80
  YAML

  depends_on = [
    helm_release.nginx_ingress_certbot,
    kubectl_manifest.letsencrypt_prod,
    kubernetes_namespace.livekit
  ]
}

