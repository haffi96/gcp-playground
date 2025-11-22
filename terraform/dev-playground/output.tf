# output "service_url" {
#   value = google_cloud_run_service.default["chat-app"].status[0].url
# }

# GKE Cluster outputs
output "gke_cluster_name" {
  description = "GKE cluster name"
  value       = google_container_cluster.default.name
}

output "gke_cluster_location" {
  description = "GKE cluster location"
  value       = google_container_cluster.default.location
}

output "gke_cluster_endpoint" {
  description = "GKE cluster endpoint"
  value       = google_container_cluster.default.endpoint
  sensitive   = true
}

output "kubectl_connection_command" {
  description = "Command to connect to the GKE cluster"
  value       = "gcloud container clusters get-credentials ${google_container_cluster.default.name} --location ${google_container_cluster.default.location} --project ${var.project_id}"
}

# LoadBalancer IP for DNS configuration
output "loadbalancer_ip" {
  description = "NGINX Ingress LoadBalancer IP (use this for DNS A record)"
  value       = try(data.kubernetes_service.nginx_ingress_lb.status[0].load_balancer[0].ingress[0].ip, "Pending - run 'terraform refresh' after a few minutes")
}

output "nginx_ingress_ip" {
  description = "NGINX Ingress LoadBalancer IP for signaling traffic"
  value       = try(data.kubernetes_service.nginx_ingress_lb.status[0].load_balancer[0].ingress[0].ip, "Pending")
}

output "turn_loadbalancer_ip" {
  description = "TURN LoadBalancer IP (fallback only)"
  value       = try(data.kubernetes_service.turn_lb.status[0].load_balancer[0].ingress[0].ip, "Pending")
}

# LiveKit configuration
output "livekit_domain" {
  description = "LiveKit server domain"
  value       = var.livekit_domain
}

output "livekit_url" {
  description = "LiveKit server URL (HTTPS)"
  value       = "https://${var.livekit_domain}"
}

output "livekit_ws_url" {
  description = "LiveKit WebSocket URL (WSS)"
  value       = "wss://${var.livekit_domain}"
}

# Redis connection
output "redis_connection_string" {
  description = "Redis connection string (internal to cluster)"
  value       = "redis.redis.svc.cluster.local:6379"
}

# LiveKit API credentials
output "livekit_api_key" {
  description = "LiveKit API Key"
  value       = "devkey"
}

output "livekit_api_secret" {
  description = "LiveKit API Secret"
  value       = random_password.livekit_secret.result
  sensitive   = true
}

# DNS Configuration Instructions
output "dns_configuration_instructions" {
  description = "Instructions for configuring DNS in Cloudflare"
  value       = <<-EOT
    ========================================
    DNS CONFIGURATION REQUIRED
    ========================================
    
    Please configure the following DNS record in Cloudflare:
    
    Type:   A
    Name:   livekit-gke
    Value:  ${try(data.kubernetes_service.nginx_ingress_lb.status[0].load_balancer[0].ingress[0].ip, "PENDING - wait for LoadBalancer IP")}
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
}

# Post-deployment commands
output "useful_commands" {
  description = "Useful kubectl commands for managing LiveKit"
  value       = <<-EOT
    # Connect to cluster:
    gcloud container clusters get-credentials ${google_container_cluster.default.name} --location ${google_container_cluster.default.location} --project ${var.project_id}
    
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
}
