#ifndef CHUNK_H
#define CHUNK_H

#include <glad/glad.h>
#include <vector>
#include <cstdint>
#include <stb_perlin.h>

// 方块类型
enum BlockType : uint8_t {
    BLOCK_AIR = 0,
    BLOCK_STONE = 1,
    BLOCK_DIRT = 2,
    BLOCK_GRASS = 3
};

class Chunk {
public:
    static const int CHUNK_SIZE = 16;
    
    // 存储方块数据：16*16*16 = 4096 个字节
    uint8_t m_blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    
    // 专门用来存"生成好的顶点"，发给 GPU 用
    std::vector<float> m_vertices;
    
    unsigned int VAO, VBO;
    
    Chunk() : VAO(0), VBO(0) {
        // 初始化所有方块为空气
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    m_blocks[x][y][z] = BLOCK_AIR;
                }
            }
        }
    }
    
    ~Chunk() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }
    }
    
    // 使用柏林噪声生成地形（需要传入区块世界坐标）
    void initData(int chunkX = 0, int chunkZ = 0) {
        // 噪声参数
        const float scale = 0.05f;      // 噪声缩放（越小地形越平缓）
        const int baseHeight = 8;       // 基础地形高度
        const int heightRange = 6;      // 高度变化范围
        
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                // 计算世界坐标
                float worldX = (float)(chunkX * CHUNK_SIZE + x);
                float worldZ = (float)(chunkZ * CHUNK_SIZE + z);
                
                // 使用柏林噪声计算地形高度
                float noiseValue = stb_perlin_fbm_noise3(
                    worldX * scale, 
                    0.0f, 
                    worldZ * scale, 
                    2.0f,   // lacunarity
                    0.5f,   // gain
                    4       // octaves
                );
                
                // 将噪声值(-1到1)映射到高度
                int terrainHeight = baseHeight + (int)(noiseValue * heightRange);
                terrainHeight = (terrainHeight < 1) ? 1 : terrainHeight;
                terrainHeight = (terrainHeight >= CHUNK_SIZE) ? CHUNK_SIZE - 1 : terrainHeight;
                
                for (int y = 0; y < CHUNK_SIZE; y++) {
                    if (y > terrainHeight) {
                        // 地表以上是空气
                        m_blocks[x][y][z] = BLOCK_AIR;
                    } else if (y == terrainHeight) {
                        // 最顶层是泥土
                        m_blocks[x][y][z] = BLOCK_DIRT;
                    } else if (y > terrainHeight - 3) {
                        // 地表下3格是泥土
                        m_blocks[x][y][z] = BLOCK_DIRT;
                    } else {
                        // 更深处是石头
                        m_blocks[x][y][z] = BLOCK_STONE;
                    }
                }
            }
        }
    }
    
    // 检查某个位置是否是空气（用于面剔除）
    bool isAir(int x, int y, int z) {
        // 边界外视为空气，这样边缘的面会被渲染
        if (x < 0 || x >= CHUNK_SIZE || 
            y < 0 || y >= CHUNK_SIZE || 
            z < 0 || z >= CHUNK_SIZE) {
            return true;
        }
        return m_blocks[x][y][z] == BLOCK_AIR;
    }
    
    // 根据方块类型和面返回纹理索引（atlas中的偏移）
    // Atlas布局(水平): dirt(0), stone(1), grass(2)
    int getTextureIndex(uint8_t blockType, int face) {
        // face: 0=前, 1=后, 2=左, 3=右, 4=底, 5=顶
        switch (blockType) {
            case BLOCK_STONE:
                return 1; // 石头所有面相同
            case BLOCK_DIRT:
                return 0; // 泥土所有面相同
            case BLOCK_GRASS:
                if (face == 5) return 2; // 顶面草
                if (face == 4) return 0; // 底面泥土
                return 0; // 侧面泥土（可后续添加草侧面贴图）
            default:
                return 0;
        }
    }

    // 添加一个面的顶点数据
    void addFace(float x, float y, float z, int face, uint8_t blockType) {
        // 每个面6个顶点（2个三角形），每个顶点5个float（位置3 + 纹理坐标2）
        // face: 0=前, 1=后, 2=左, 3=右, 4=底, 5=顶

        // 计算atlas UV偏移
        const int ATLAS_TILES = 3; // atlas中有3个贴图
        int texIndex = getTextureIndex(blockType, face);
        float tileWidth = 1.0f / ATLAS_TILES;
        float uOffset = texIndex * tileWidth;

        // 定义本地UV（0~1范围），之后映射到atlas
        float localUV[6][6][2] = {
            // 前面 (z+)
            {{0,0},{1,0},{1,1},{1,1},{0,1},{0,0}},
            // 后面 (z-)
            {{0,0},{1,1},{1,0},{1,1},{0,0},{0,1}},
            // 左面 (x-)
            {{1,1},{0,1},{0,0},{0,0},{1,0},{1,1}},
            // 右面 (x+)
            {{0,1},{1,0},{1,1},{1,0},{0,1},{0,0}},
            // 底面 (y-)
            {{0,1},{1,1},{1,0},{1,0},{0,0},{0,1}},
            // 顶面 (y+)
            {{0,1},{1,0},{1,1},{1,0},{0,1},{0,0}}
        };

        float positions[6][6][3] = {
            // 前面 (z+)
            {{x,y,z+1},{x+1,y,z+1},{x+1,y+1,z+1},{x+1,y+1,z+1},{x,y+1,z+1},{x,y,z+1}},
            // 后面 (z-)
            {{x,y,z},{x+1,y+1,z},{x+1,y,z},{x+1,y+1,z},{x,y,z},{x,y+1,z}},
            // 左面 (x-)
            {{x,y+1,z+1},{x,y+1,z},{x,y,z},{x,y,z},{x,y,z+1},{x,y+1,z+1}},
            // 右面 (x+)
            {{x+1,y+1,z+1},{x+1,y,z},{x+1,y+1,z},{x+1,y,z},{x+1,y+1,z+1},{x+1,y,z+1}},
            // 底面 (y-)
            {{x,y,z},{x+1,y,z},{x+1,y,z+1},{x+1,y,z+1},{x,y,z+1},{x,y,z}},
            // 顶面 (y+)
            {{x,y+1,z},{x+1,y+1,z+1},{x+1,y+1,z},{x+1,y+1,z+1},{x,y+1,z},{x,y+1,z+1}}
        };

        for (int i = 0; i < 6; i++) {
            // 位置
            m_vertices.push_back(positions[face][i][0]);
            m_vertices.push_back(positions[face][i][1]);
            m_vertices.push_back(positions[face][i][2]);
            // 纹理坐标（映射到atlas）
            float u = uOffset + localUV[face][i][0] * tileWidth;
            float v = localUV[face][i][1];
            m_vertices.push_back(u);
            m_vertices.push_back(v);
        }
    }
    
    // 【核心】构建网格
    void updateMesh() {
        m_vertices.clear();
        
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    // 如果当前方块是空气，跳过
                    if (m_blocks[x][y][z] == BLOCK_AIR) {
                        continue;
                    }
                    
                    uint8_t blockType = m_blocks[x][y][z];
                    
                    // 检查每个面是否需要渲染（相邻方块是否为空气）
                    // 前面 (z+)
                    if (isAir(x, y, z + 1)) {
                        addFace((float)x, (float)y, (float)z, 0, blockType);
                    }
                    // 后面 (z-)
                    if (isAir(x, y, z - 1)) {
                        addFace((float)x, (float)y, (float)z, 1, blockType);
                    }
                    // 左面 (x-)
                    if (isAir(x - 1, y, z)) {
                        addFace((float)x, (float)y, (float)z, 2, blockType);
                    }
                    // 右面 (x+)
                    if (isAir(x + 1, y, z)) {
                        addFace((float)x, (float)y, (float)z, 3, blockType);
                    }
                    // 底面 (y-)
                    if (isAir(x, y - 1, z)) {
                        addFace((float)x, (float)y, (float)z, 4, blockType);
                    }
                    // 顶面 (y+)
                    if (isAir(x, y + 1, z)) {
                        addFace((float)x, (float)y, (float)z, 5, blockType);
                    }
                }
            }
        }
        
        // 创建或更新 VAO/VBO
        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
        }
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), 
                     m_vertices.data(), GL_STATIC_DRAW);
        
        // 位置属性 (3 floats)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // 纹理坐标属性 (2 floats)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    // 绘制函数
    void render() {
        if (m_vertices.empty()) return;
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, m_vertices.size() / 5);
        glBindVertexArray(0);
    }
};

#endif
