# output "service_url" {
#   value = google_cloud_run_service.default["chat-app"].status[0].url
# }

locals {
  livekit_stack_enabled = var.enable_livekit && length(google_container_cluster.default) > 0

  livekit_cluster_name     = local.livekit_stack_enabled ? google_container_cluster.default[0].name : null
  livekit_cluster_location = local.livekit_stack_enabled ? google_container_cluster.default[0].location : null
  livekit_cluster_endpoint = local.livekit_stack_enabled ? google_container_cluster.default[0].endpoint : null
  livekit_api_secret       = local.livekit_stack_enabled ? random_password.livekit_secret[0].result : null

  nginx_ingress_lb_ip = local.livekit_stack_enabled && length(data.kubernetes_service.nginx_ingress_lb) > 0 ? try(data.kubernetes_service.nginx_ingress_lb[0].status[0].load_balancer[0].ingress[0].ip, null) : null
  turn_lb_ip          = local.livekit_stack_enabled && length(data.kubernetes_service.turn_lb) > 0 ? try(data.kubernetes_service.turn_lb[0].status[0].load_balancer[0].ingress[0].ip, null) : null
}

# GKE Cluster outputs
output "gke_cluster_name" {
  description = "GKE cluster name"
  value       = local.livekit_cluster_name
}

output "gke_cluster_location" {
  description = "GKE cluster location"
  value       = local.livekit_cluster_location
}

output "gke_cluster_endpoint" {
  description = "GKE cluster endpoint"
  value       = local.livekit_cluster_endpoint
  sensitive   = true
}

output "kubectl_connection_command" {
  description = "Command to connect to the GKE cluster"
  value       = local.livekit_stack_enabled ? "gcloud container clusters get-credentials ${local.livekit_cluster_name} --location ${local.livekit_cluster_location} --project ${var.project_id}" : null
}

# LoadBalancer IP for DNS configuration
output "loadbalancer_ip" {
  description = "NGINX Ingress LoadBalancer IP (use this for DNS A record)"
  value       = local.livekit_stack_enabled ? coalesce(local.nginx_ingress_lb_ip, "Pending - run 'terraform refresh' after a few minutes") : null
}

output "nginx_ingress_ip" {
  description = "NGINX Ingress LoadBalancer IP for signaling traffic"
  value       = local.livekit_stack_enabled ? coalesce(local.nginx_ingress_lb_ip, "Pending") : null
}

output "turn_loadbalancer_ip" {
  description = "TURN LoadBalancer IP (fallback only)"
  value       = local.livekit_stack_enabled ? coalesce(local.turn_lb_ip, "Pending") : null
}

# LiveKit configuration
output "livekit_domain" {
  description = "LiveKit server domain"
  value       = var.enable_livekit ? var.livekit_domain : null
}

output "livekit_url" {
  description = "LiveKit server URL (HTTPS)"
  value       = var.enable_livekit ? "https://${var.livekit_domain}" : null
}

output "livekit_ws_url" {
  description = "LiveKit WebSocket URL (WSS)"
  value       = var.enable_livekit ? "wss://${var.livekit_domain}" : null
}

# Redis connection
output "redis_connection_string" {
  description = "Redis connection string (internal to cluster)"
  value       = var.enable_livekit ? "redis.redis.svc.cluster.local:6379" : null
}

# LiveKit API credentials
output "livekit_api_key" {
  description = "LiveKit API Key"
  value       = var.enable_livekit ? "devkey" : null
}

output "livekit_api_secret" {
  description = "LiveKit API Secret"
  value       = local.livekit_api_secret
  sensitive   = true
}

# DNS Configuration Instructions
output "dns_configuration_instructions" {
  description = "Instructions for configuring DNS in Cloudflare"
  value       = local.livekit_stack_enabled ? (
    <<-EOT
    ========================================
    DNS CONFIGURATION REQUIRED
    ========================================
    
    Please configure the following DNS record in Cloudflare:
    
    Type:   A
    Name:   livekit-gke
    Value:  ${coalesce(local.nginx_ingress_lb_ip, "PENDING - wait for LoadBalancer IP")}
    TTL:    Auto
    Proxy:  OFF (Disable Cloudflare proxy - DNS only)
    
    IMPORTANT: 
    - The orange cloud icon in Cloudflare MUST be turned OFF (gray)
    - LiveKit requires direct connection for WebRTC/UDP traffic
    - Cloudflare proxy only supports HTTP/HTTPS, not UDP
    
    Domain: ${var.livekit_domain}
    
    After configuring DNS:
    1. Wait 2-3 minutes for DNS propagation
    2. cert-manager will automatically issue SSL certificates (5-10 min)
    3. Test LiveKit: https://${var.livekit_domain}
    4. Check certificate: kubectl get certificate -n livekit
    5. Check service status: kubectl get svc -n livekit
    
    ========================================
    EOT
  ) : "LiveKit is disabled. Set enable_livekit=true to deploy and receive DNS instructions."
}

# Post-deployment commands
output "useful_commands" {
  description = "Useful kubectl commands for managing LiveKit"
  value       = local.livekit_stack_enabled ? (
    <<-EOT
    # Connect to cluster:
    gcloud container clusters get-credentials ${google_container_cluster.default[0].name} --location ${google_container_cluster.default[0].location} --project ${var.project_id}
    
    # IMPORTANT: Fix LiveKit probe timing (Helm chart bug workaround):
    # Run this after every 'terraform apply' that updates LiveKit:
    kubectl patch deployment -n livekit livekit-server --type='json' -p='[{"op": "add", "path": "/spec/template/spec/containers/0/livenessProbe/initialDelaySeconds", "value": 30},{"op": "add", "path": "/spec/template/spec/containers/0/readinessProbe/initialDelaySeconds", "value": 10}]'
    
    # Check LiveKit pods:
    kubectl get pods -n livekit
    
    # Check LiveKit logs:
    kubectl logs -n livekit -l app.kubernetes.io/name=livekit-server --tail=100 -f
    
    # Check SSL certificate status:
    kubectl get certificate -n livekit
    kubectl describe certificate livekit-tls -n livekit
    
    # Check LiveKit service and LoadBalancer IP:
    kubectl get svc -n livekit
    
    # Check nodes and pods:
    kubectl get nodes -o wide
    kubectl get pods -n livekit -o wide
    
    # Scale LiveKit to zero (to save costs):
    kubectl scale deployment -n livekit livekit-server --replicas=0
    
    # Scale LiveKit back up:
    kubectl scale deployment -n livekit livekit-server --replicas=1
    
    # Delete cluster (to stop all costs):
    terraform destroy
    EOT
  ) : "LiveKit is disabled. Set enable_livekit=true to deploy and view LiveKit management commands."
}
