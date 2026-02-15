# Redis namespace
resource "kubernetes_namespace" "redis" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name = "redis"
  }

  depends_on = [google_container_node_pool.livekit_nodes[0]]
}

# Redis ConfigMap for configuration
resource "kubernetes_config_map" "redis_config" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name      = "redis-config"
    namespace = kubernetes_namespace.redis[0].metadata[0].name
  }

  data = {
    "redis.conf" = <<-EOT
      maxmemory 256mb
      maxmemory-policy allkeys-lru
      save ""
      appendonly no
    EOT
  }
}

# Redis StatefulSet
resource "kubernetes_stateful_set" "redis" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name      = "redis"
    namespace = kubernetes_namespace.redis[0].metadata[0].name
  }

  spec {
    service_name = "redis"
    replicas     = 1

    selector {
      match_labels = {
        app = "redis"
      }
    }

    template {
      metadata {
        labels = {
          app = "redis"
        }
      }

      spec {
        container {
          name  = "redis"
          image = "redis:${var.redis_version}"

          port {
            container_port = 6379
            name           = "redis"
          }

          command = ["redis-server"]
          args    = ["/etc/redis/redis.conf"]

          volume_mount {
            name       = "redis-config"
            mount_path = "/etc/redis"
          }

          volume_mount {
            name       = "redis-data"
            mount_path = "/data"
          }

          resources {
            requests = {
              cpu    = "100m"
              memory = "256Mi"
            }
            limits = {
              cpu    = "500m"
              memory = "512Mi"
            }
          }

          liveness_probe {
            tcp_socket {
              port = 6379
            }
            initial_delay_seconds = 30
            period_seconds        = 10
          }

          readiness_probe {
            exec {
              command = ["redis-cli", "ping"]
            }
            initial_delay_seconds = 5
            period_seconds        = 5
          }
        }

        volume {
          name = "redis-config"
          config_map {
            name = kubernetes_config_map.redis_config[0].metadata[0].name
          }
        }

        volume {
          name = "redis-data"
          empty_dir {}
        }
      }
    }
  }
}

# Redis Service
resource "kubernetes_service" "redis" {
  count = var.enable_livekit ? 1 : 0

  metadata {
    name      = "redis"
    namespace = kubernetes_namespace.redis[0].metadata[0].name
  }

  spec {
    selector = {
      app = "redis"
    }

    port {
      port        = 6379
      target_port = 6379
      protocol    = "TCP"
    }

    type = "ClusterIP"
  }
}

