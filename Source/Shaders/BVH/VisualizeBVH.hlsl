#include "../ShaderInterop.hpp"
#include "../RayTrace.hlsli"

ConstantBuffer<Camera> camera : register(b0);
StructuredBuffer<BVHNode> tlas : register(t1);
StructuredBuffer<BVHNode> blas[] : register(t2);
ConstantBuffer<Instance> instances[] : register(b3);
RWTexture2D<float4> result : register(u4);

struct CSParam
{
    uint3 DispatchThreadID : SV_DispatchThreadID;
    uint3 GroupThreadID : SV_GroupThreadID;
    uint3 GroupID : SV_GroupID;
    uint GroupIndex : SV_GroupIndex;
};

bool TLASTraversal(RayDesc ray, out float depth, out uint instance_id)
{
    uint node = 0;
    float min_t = 1e32;
    float t = 0.0;
    float current_depth = 0.0;
    bool find_leaf = false;
    float max_depth = 0.0;
    bool terminal = false;
    
    while (true)
    {
        max_depth = max(max_depth, current_depth);
        
        if (Intersection(tlas[node].aabb, ray, t))
        {
            if (tlas[node].left_child == ~0U &&
                tlas[node].right_child == ~0U)
            {
                if (min_t > t)
                {
                    min_t = t;
                    depth = current_depth;
                    find_leaf = true;
                    instance_id = tlas[node].prim_id;
                }
            }
            else
            {
                node = tlas[node].left_child;
                current_depth += 1.0;
                continue;
            }
        }
        
        while (true)
        {
            uint parent = tlas[node].parent;
            
            if (parent != ~0U && node == tlas[parent].left_child)
            {
                if (tlas[parent].right_child != ~0U)
                {
                    node = tlas[parent].right_child;
                    break;
                }
            }
            
            node = parent;
            current_depth -= 1.0;
            if (node == ~0U)
            {
                terminal = true;
                break;
            }
        }
        
        if (terminal)
        {
            break;
        }
    }
    
    if (!find_leaf)
    {
        depth = max_depth;
        instance_id = ~0U;
        return false;
    }
    
    return true;
}

bool BLASTraversal(RayDesc ray, uint instance_id, out float depth)
{
    uint node = 0;
    float min_t = 1e32;
    float t = 0.0;
    float current_depth = 0.0;
    bool find_leaf = false;
    float max_depth = 0.0;
    bool terminal = false;
    
    while (true)
    {
        max_depth = max(max_depth, current_depth);
        
        if (Intersection(blas[instance_id][node].aabb.Transform(instances[instance_id].transform), ray, t))
        {
            if (blas[instance_id][node].left_child == ~0U &&
                blas[instance_id][node].right_child == ~0U)
            {
                if (min_t > t)
                {
                    min_t = t;
                    depth = current_depth;
                    find_leaf = true;
                }
            }
            else
            {
                node = blas[instance_id][node].left_child;
                current_depth += 1.0;
                continue;
            }
        }
        
        while (true)
        {
            uint parent = blas[instance_id][node].parent;
            
            if (parent != ~0U && node == blas[instance_id][parent].left_child)
            {
                if (blas[instance_id][parent].right_child != ~0U)
                {
                    node = blas[instance_id][parent].right_child;
                    break;
                }
            }
            
            node = parent;
            current_depth -= 1.0;
            if (node == ~0U)
            {
                terminal = true;
                break;
            }
        }
        
        if (terminal)
        {
            break;
        }
    }
    
    if (!find_leaf)
    {
        depth = max_depth;
        return false;
    }
    return true;
}

[numthreads(32, 32, 1)]
void main(CSParam param)
{
    uint2 extent;
    result.GetDimensions(extent.x, extent.y);

    if (param.DispatchThreadID.x > extent.x || param.DispatchThreadID.y > extent.y)
    {
        return;
    }
        
    float2 screen_coords = (float2(param.DispatchThreadID.xy) + 0.5) / float2(extent);
    screen_coords.y = 1.0 - screen_coords.y;
    screen_coords = screen_coords * 2.0 - 1.0;
    
    RayDesc ray = camera.CastRay(screen_coords);
    
    float depth = 0.0;
        
    float3 color = 0.0;
    float max_depth = 0.0;
    
    uint instance_id;
    if (TLASTraversal(ray, depth, instance_id))
    {
        color = 1.0;
        BLASTraversal(ray, instance_id, depth);
        
        uint item_count = 0;
        uint item_stride = 0;
        blas[instance_id].GetDimensions(item_count, item_stride);
        max_depth = log2(item_count) * 2 + 1;
    }
    else
    {
        color = float3(1.0, 0.0, 0.0);
        
        uint item_count = 0;
        uint item_stride = 0;
        tlas.GetDimensions(item_count, item_stride);
        max_depth = log2(item_count) * 2 + 1;
        
        if (depth == 0)
        {
            result[param.DispatchThreadID.xy] = float4(0.0, 1.0, 0.0, 1.0);
            return;
        }
    }
        
    color *= depth / max_depth;
    result[param.DispatchThreadID.xy] = float4(color, 1.0);
}