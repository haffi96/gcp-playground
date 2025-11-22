# LiveKit on GKE - Infrastructure Setup

This Terraform configuration deploys a complete LiveKit infrastructure on Google Kubernetes Engine (GKE) with the following components:

## Components

- **GKE Standard Cluster** (europe-west2)
  - Auto-scaling node pool (0-1 nodes)
  - Spot VMs for cost optimization
  - Host networking support for LiveKit
- **Self-hosted Redis** (in-cluster)
  - StatefulSet deployment
  - Ephemeral storage (deleted with cluster)
- **cert-manager** with Let's Encrypt
  - Automated SSL/TLS certificate management
  - Certificates issued via HTTP-01 challenge
  - Production and staging issuers
- **LiveKit Server** via Helm
  - WebRTC server for real-time communication
  - HTTPS/WSS for signaling (port 443)
  - Direct UDP for media (ports 50000-50200)
  - TURN/TLS enabled for firewall traversal (port 5349)
  - LoadBalancer service for L4 direct access
  - Host networking enabled for ultra-low latency
  - Domain: `livekit-gke.haffi.dev`

## Prerequisites

1. **GCP Project**: `playground-442622` (configured in variables)
2. **Domain**: `haffi.dev` (owned on Cloudflare)
3. **Tools**:
   - Terraform >= 1.0
   - gcloud CLI
   - kubectl

## Deployment Steps

### 1. Initialize Terraform

```bash
cd terraform/dev-playground
terraform init
```

### 2. Review the Plan

```bash
terraform plan
```

### 3. Apply the Configuration

```bash
terraform apply
```

This will take approximately 10-15 minutes to:
- Create the GKE cluster
- Deploy Redis, cert-manager, NGINX Ingress, and LiveKit
- Provision SSL certificates

### 4. Get the LoadBalancer IP

After deployment completes, get the NGINX Ingress LoadBalancer IP:

```bash
# Get NGINX Ingress IP (for DNS)
terraform output nginx_ingress_ip

# View all outputs
terraform output
```

The NGINX Ingress LoadBalancer typically gets its IP within 2-3 minutes.

### 5. Configure DNS in Cloudflare

Create an A record in Cloudflare for your domain:

- **Type**: A
- **Name**: `livekit-gke`
- **Value**: Get from `terraform output nginx_ingress_ip`
- **TTL**: Auto
- **Proxy**: **OFF** (Gray cloud - DNS only)

**Important**: The Cloudflare proxy MUST be disabled because:
- LiveKit requires direct UDP access for WebRTC media
- TURN server needs direct connection
- Cloudflare proxy only supports HTTP/HTTPS, not UDP

**DNS Propagation**: After configuring, DNS can take 5-30 minutes to propagate. Test using:
```bash
# Check DNS propagation
dig @1.1.1.1 livekit-gke.haffi.dev +short

# Test with IP while waiting
curl -k https://<NGINX_IP> -H "Host: livekit-gke.haffi.dev"
```

### 6. Verify DNS Configuration

Wait 2-5 minutes for DNS propagation, then verify:

```bash
dig livekit-gke.haffi.dev
```

### 7. Wait for SSL Certificates

cert-manager will automatically request SSL certificates from Let's Encrypt using HTTP-01 challenge. This takes 5-10 minutes:

```bash
# Check certificate status
kubectl get certificate -n livekit

# Wait for "Ready: True"
kubectl describe certificate livekit-tls -n livekit

# Check cert-manager logs if issues
kubectl logs -n cert-manager -l app=cert-manager --tail=50
```

### 8. Access LiveKit

Once DNS and SSL are configured:

```bash
# Test HTTPS endpoint
curl https://livekit-gke.haffi.dev

# Test with verbose SSL info
curl -v https://livekit-gke.haffi.dev
```

You should see a LiveKit server response with valid SSL.

## Performance & Latency Optimization

This setup is optimized for **ultra-low latency** WebRTC:

### Network Path Analysis

**Media Path (UDP):**
```
Client → Internet → GCP LB → Node (hostNetwork) → LiveKit Process
         ~20-50ms     ~1-2ms        0ms (direct)
         
Total added latency: ~21-52ms (mostly Internet, not infrastructure)
```

**Signaling Path (WebSocket):**
```
Client → Internet → GCP LB → Node → LiveKit Process (TLS termination)
         ~20-50ms     ~1-2ms    0ms
```

### Optimization Techniques Used

1. **Host Networking** (`hostNetwork: true`)
   - Eliminates CNI plugin overhead
   - No iptables NAT traversal
   - Direct binding to node ports
   - **Savings: 1-5ms per packet**

2. **L4 LoadBalancer** (not L7 Ingress)
   - Pass-through UDP forwarding
   - No HTTP inspection
   - Stateless for UDP
   - **Savings: 2-10ms per connection**

3. **No Reverse Proxy**
   - No additional TLS termination hops
   - Direct TLS at application
   - **Savings: 5-15ms**

4. **Regional Deployment** (europe-west2)
   - Close to target audience
   - Consider multi-region for global users

5. **Spot VMs**
   - Same performance as regular VMs
   - No performance penalty
   - **Cost savings: 60-91%**

### Latency Benchmarking

After deployment, test latency:

```bash
# WebSocket latency
wscat -c wss://livekit-gke.haffi.dev

# TURN server latency
stunsc -p 3478 livekit-gke.haffi.dev

# Check node network performance
kubectl run -it --rm netperf --image=networkstatic/iperf3 --restart=Never
```

### Expected Performance

- **WebRTC Media Latency**: 50-150ms (mostly Internet)
- **Infrastructure Overhead**: <5ms
- **Signaling Latency**: 50-100ms
- **Packet Loss**: <0.1% (GCP network)
- **Jitter**: <10ms

## Cost Management

### When Active
- **Control Plane**: ~$73/month (always running)
- **Worker Nodes**: Variable based on usage
  - With Spot VMs: ~$25-50/month per node
  - With auto-scaling to 0: $0 when no pods running

### Cost-Saving Strategies

#### Option 1: Scale to Zero (Keep Cluster)
Scale LiveKit to zero replicas when not in use:

```bash
kubectl scale deployment -n livekit livekit-server --replicas=0
```

Node pool will auto-scale to 0 nodes. You'll only pay ~$73/month for the control plane.

To scale back up:

```bash
kubectl scale deployment -n livekit livekit-server --replicas=1
```

#### Option 2: Delete the Entire Cluster
When not testing LiveKit:

```bash
terraform destroy
```

This removes everything and costs $0 when deleted.

To recreate:

```bash
terraform apply
```

Takes 10-15 minutes to recreate. You'll need to reconfigure DNS if the LoadBalancer IP changes.

## Useful Commands

### Connect to Cluster

```bash
gcloud container clusters get-credentials dev-playground \
  --location europe-west2 \
  --project playground-442622
```

### Check LiveKit Status

```bash
# Check pods
kubectl get pods -n livekit

# View logs
kubectl logs -n livekit -l app.kubernetes.io/name=livekit-server --tail=100 -f

# Check ingress
kubectl get ingress -n livekit
```

### Check LoadBalancer IP and Service

```bash
kubectl get svc -n livekit livekit-server
```

### Access Redis (from within cluster)

```bash
kubectl run -it --rm redis-client --image=redis:7.2-alpine --restart=Never -- sh
# redis-cli -h redis.redis.svc.cluster.local ping
```

## LiveKit Configuration

### API Credentials

Get your LiveKit API credentials:

```bash
# API Key (default for dev)
terraform output livekit_api_key

# API Secret (randomly generated)
terraform output livekit_api_secret
```

Use these credentials in your LiveKit client applications.

### WebSocket URL

```bash
terraform output livekit_ws_url
```

Returns: `wss://livekit-gke.haffi.dev` (Secure WebSocket with TLS)

## Troubleshooting

### LoadBalancer IP Pending

If the LoadBalancer IP shows "Pending":

```bash
kubectl get svc -n ingress-nginx
# Wait 2-3 minutes, then:
terraform refresh
terraform output loadbalancer_ip
```

### LiveKit Not Responding

1. Check if pods are running:
   ```bash
   kubectl get pods -n livekit
   ```

2. Check logs:
   ```bash
   kubectl logs -n livekit -l app.kubernetes.io/name=livekit-server
   ```

3. Verify Redis is running:
   ```bash
   kubectl get pods -n redis
   ```

4. Check node status (hostNetwork requires nodes):
   ```bash
   kubectl get nodes
   kubectl describe pod -n livekit <pod-name>
   ```

5. Verify LoadBalancer provisioned:
   ```bash
   kubectl get svc -n livekit livekit-server
   # Wait for EXTERNAL-IP to show (not <pending>)
   ```

### DNS Issues

Verify DNS is pointing to the correct IP:

```bash
dig livekit-gke.haffi.dev
```

Ensure:
- A record exists
- Points to LoadBalancer IP
- Cloudflare proxy is OFF (gray cloud)

## Architecture

```
Internet
   |
   v
Cloudflare DNS (livekit-gke.haffi.dev) [DNS Only - Proxy OFF]
   |
   v
GCP LoadBalancer (L4 - TCP/UDP)
   |
   +---[HTTPS:443]-----------> LiveKit Server (hostNetwork=true)
   |   - HTTPS/WSS Signaling
   |   - TLS at pod level
   |
   +---[UDP:50000-50200]-----> LiveKit Server
   |   - WebRTC Media (DTLS-SRTP encrypted)
   |   - Direct path, no hops
   |
   +---[TURN:5349]-----------> LiveKit TURN Server
       - TLS encrypted
       - NAT/Firewall traversal
   
LiveKit Server
   |
   v
Redis (StatefulSet)

SSL Certificates:
  cert-manager → Let's Encrypt → Kubernetes Secret → LiveKit Pod
```

**Key Design Points for Low Latency:**

1. **hostNetwork: true** - LiveKit pods bind directly to node ports
   - Bypasses kube-proxy and iptables
   - Zero network virtualization overhead
   - Direct kernel-level packet handling

2. **GCP L4 LoadBalancer** - Layer 4 (TCP/UDP) pass-through
   - No application-level inspection
   - Minimal latency overhead (~1-2ms)
   - Direct packet forwarding

3. **No Ingress/Reverse Proxy** - WebRTC media path
   - UDP flows directly: Client → LB → Pod
   - Only 2 hops: LB + Pod
   - No SSL termination on UDP (native DTLS-SRTP)

4. **TLS at Pod Level** - LiveKit handles SSL directly
   - No external SSL termination
   - Certificates from cert-manager
   - HTTP/2 for signaling efficiency

5. **One Pod Per Node** - Required for hostNetwork
   - Port conflicts prevented
   - Dedicated node resources
   - Optimal for single-tenant workloads

## Files

- `gke.tf` - GKE cluster and node pool configuration
- `redis.tf` - Self-hosted Redis deployment
- `ssl.tf` - cert-manager and Let's Encrypt configuration
- `livekit.tf` - LiveKit Helm chart deployment
- `livekit-values.yaml` - LiveKit Helm values template
- `providers.tf` - Terraform provider configuration
- `variables.tf` - Input variables
- `output.tf` - Output values

## Security Notes

1. **API Credentials**: The default setup uses a dev key. For production, generate secure keys.
2. **Redis**: Uses ephemeral storage. For production, use persistent volumes.
3. **Node Security**: Nodes use Workload Identity for GCP service authentication.
4. **Network**: Cluster uses default VPC network. For production, use a custom VPC.
5. **SSL/TLS**: Automatic certificate management via Let's Encrypt
   - Certificates auto-renewed every 90 days
   - HTTPS for signaling, DTLS-SRTP for media
   - TURN/TLS available for strict firewalls
6. **Firewall**: Ensure GCP firewall rules allow:
   - TCP 443 (HTTPS)
   - TCP 80 (HTTP redirect)
   - UDP 50000-50200 (WebRTC media)
   - TCP/UDP 5349 (TURN/TLS)
   - UDP 3478 (TURN/UDP)

## Customization

### Change Node Pool Size

Edit `variables.tf`:

```hcl
variable "gke_node_pool_max_count" {
  default = 5  # Increase for more capacity
}
```

### Change Machine Type

Edit `variables.tf`:

```hcl
variable "gke_machine_type" {
  default = "n2-standard-4"  # More powerful machines
}
```

### Change LiveKit Version

Edit `variables.tf`:

```hcl
variable "livekit_version" {
  default = "1.6.0"  # Update to newer version
}
```

Then run:

```bash
terraform apply
```

## Support

For LiveKit documentation: https://docs.livekit.io/

For issues with this infrastructure, check the Terraform output and Kubernetes logs.

