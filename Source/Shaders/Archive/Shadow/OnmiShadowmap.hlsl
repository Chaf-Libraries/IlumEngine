#include "../ShaderInterop.hpp"
#include "../Culling.hlsli"
#include "../Common.hlsli"

ConstantBuffer<Instance> instances[] : register(b0);
StructuredBuffer<Meshlet> meshlets[] : register(t1);
StructuredBuffer<Vertex> vertices[] : register(t2);
StructuredBuffer<uint> meshlet_vertices[] : register(t3);
StructuredBuffer<uint> meshlet_primitives[] : register(t4);
#ifdef ALPHA_TEST
ConstantBuffer<Material> materials[] : register(b5);
Texture2D<float4> texture_array[] : register(t6);
SamplerState texture_sampler : register(s7);
#endif

struct CSParam
{
    uint DispatchThreadID : SV_DispatchThreadID;
    uint GroupThreadID : SV_GroupThreadID;
    uint GroupID : SV_GroupID;
};

struct VertexOut
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD0;
    float3 PositionVS : POSITIONT0;
};

struct PrimitiveOut
{
    uint InstanceID : COLOR1;
    uint PrimitiveID : COLOR2;
    uint MeshletID : COLOR3;
    uint Layer : SV_RenderTargetArrayIndex;
};

struct Payload
{
    uint meshletIndices[32];
};

[[vk::push_constant]]
struct
{
    float4x4 view_projection;
    float3 position;
    uint face_id;
    uint instance_id;
    uint meshlet_count;
} push_constants;

groupshared Payload shared_payload;
[numthreads(32, 1, 1)]
void ASmain(CSParam param)
{
    bool visible = false;

    if (param.DispatchThreadID.x < push_constants.meshlet_count)
    {
        Meshlet meshlet = meshlets[instances[push_constants.instance_id].mesh][param.DispatchThreadID.x];
        Camera cam;
        cam.view_projection = push_constants.view_projection;
        cam.position = push_constants.position;
        visible = IsMeshletVisible(meshlet, instances[push_constants.instance_id].transform, cam);
    }

    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        shared_payload.meshletIndices[index] = param.DispatchThreadID.x;
    }

    uint visible_count = WaveActiveCountBits(visible);

    DispatchMesh(visible_count, 1, 1, shared_payload);
}

[outputtopology("triangle")]
[numthreads(32, 1, 1)]
void MSmain(CSParam param, in payload Payload pay_load, out vertices VertexOut verts[64], out indices uint3 tris[124], out primitives PrimitiveOut prims[124])
{
    uint meshlet_index = pay_load.meshletIndices[param.GroupID.x];

    if (meshlet_index >= push_constants.meshlet_count)
    {
        return;
    }

    float4x4 transform = instances[push_constants.instance_id].transform;
    
    Meshlet meshlet = meshlets[instances[push_constants.instance_id].mesh][meshlet_index];

    SetMeshOutputCounts(meshlet.vertex_count, meshlet.primitive_count);

    for (uint i = param.GroupThreadID.x; i < meshlet.vertex_count; i += 32)
    {
        uint vertex_index = meshlet_vertices[instances[push_constants.instance_id].mesh][meshlet.vertex_offset + i];
        Vertex vertex = vertices[instances[push_constants.instance_id].mesh][vertex_index];

        verts[i].Position = mul(push_constants.view_projection, mul(transform, float4(vertex.position.xyz, 1.0)));
        verts[i].Texcoord = vertex.texcoord.xy;
        verts[i].PositionVS = mul(transform, float4(vertex.position.xyz, 1.0)).xyz;
    }

    for (i = param.GroupThreadID.x; i < meshlet.primitive_count; i += 32)
    {
        prims[i].InstanceID = instances[push_constants.instance_id].id;
        prims[i].MeshletID = meshlet_index;
        prims[i].PrimitiveID = i;
        prims[i].Layer = push_constants.face_id;
        
        uint v0, v1, v2;
        UnPackTriangle(meshlet_primitives[instances[push_constants.instance_id].mesh][i + meshlet.primitive_offset], v0, v1, v2);
        
        tris[i] = uint3(v0, v1, v2);
    }
}

float PSmain(VertexOut verts, PrimitiveOut prims) : SV_Depth
{
#ifdef ALPHA_TEST
    if (instances[push_constants.instance_id].material <1024)
    {
        if (materials[instances[push_constants.instance_id].material].type == MetalRoughnessWorkflow)
        {
            float alpha = materials[instances[push_constants.instance_id].material].pbr_base_color_factor.a;
            uint albedo_tex_id = materials[instances[push_constants.instance_id].material].pbr_base_color_texture;
            if (albedo_tex_id < 1024)
            {
                float4 albedo = texture_array[albedo_tex_id].Sample(texture_sampler, verts.Texcoord);
                alpha *= albedo.a;
            }
            if (alpha < materials[instances[push_constants.instance_id].material].alpha_cut_off)
            {
                discard;
            }
        }
        else if (materials[instances[push_constants.instance_id].material].type == SpecularGlossinessWorkflow)
        {
            float alpha = materials[instances[push_constants.instance_id].material].pbr_diffuse_factor.a;
            uint albedo_tex_id = materials[instances[push_constants.instance_id].material].pbr_diffuse_texture;
            if (albedo_tex_id < 1024)
            {
                float4 albedo = texture_array[albedo_tex_id].Sample(texture_sampler, verts.Texcoord);
                alpha *= albedo.a;
            }
            if (alpha < materials[instances[push_constants.instance_id].material].alpha_cut_off)
            {
                discard;
            }
        }
    }
#endif   
    
    return (length(verts.Position.xyz - push_constants.position.xyz) + 0.01) / 100.0;
}