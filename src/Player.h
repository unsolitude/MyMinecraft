#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>
#include "Chunk.h"
#include <map>

// AABB 包围盒结构
struct AABB {
    glm::vec3 min;  // 包围盒最小坐标
    glm::vec3 max;  // 包围盒最大坐标
    
    AABB() : min(0.0f), max(0.0f) {}
    AABB(glm::vec3 position, float width, float height, float depth) {
        // 玩家中心点在position，包围盒围绕中心点
        min = glm::vec3(position.x - width/2.0f, position.y, position.z - depth/2.0f);
        max = glm::vec3(position.x + width/2.0f, position.y + height, position.z + depth/2.0f);
    }
    
    // 检测两个AABB是否相交
    bool intersects(const AABB& other) const {
        return (min.x < other.max.x && max.x > other.min.x) &&
               (min.y < other.max.y && max.y > other.min.y) &&
               (min.z < other.max.z && max.z > other.min.z);
    }
};

class Player {
public:
    // 玩家属性
    glm::vec3 position;      // 玩家位置（脚底中心点）
    glm::vec3 velocity;      // 速度
    
    // 玩家尺寸（高、宽、深）
    static constexpr float HEIGHT = 1.8f;
    static constexpr float WIDTH = 0.6f;
    static constexpr float DEPTH = 0.6f;
    
    // 物理参数
    static constexpr float GRAVITY = -20.0f;      // 重力加速度
    static constexpr float JUMP_STRENGTH = 8.0f;  // 跳跃力度
    static constexpr float TERMINAL_VELOCITY = -50.0f; // 最大下落速度
    
    bool onGround;  // 是否在地面上
    
    Player(glm::vec3 startPos) 
        : position(startPos), velocity(0.0f), onGround(false) {}
    
    // 获取玩家的AABB包围盒
    AABB getAABB() const {
        return AABB(position, WIDTH, HEIGHT, DEPTH);
    }
    
    // 获取眼睛位置（用于摄像机）
    glm::vec3 getEyePosition() const {
        return position + glm::vec3(0.0f, 1.62f, 0.0f); // 眼睛高度约1.62米
    }
    
    // 检查指定位置的方块是否为实心（非空气）
    bool isBlockSolid(int x, int y, int z, std::map<std::pair<int, int>, Chunk*>& chunks) {
        // 计算方块所在的区块坐标
        int chunkX = (x >= 0) ? x / 16 : (x - 15) / 16;
        int chunkZ = (z >= 0) ? z / 16 : (z - 15) / 16;
        
        // 计算方块在区块内的局部坐标
        int localX = x - chunkX * 16;
        int localZ = z - chunkZ * 16;
        
        // 查找对应的区块
        auto it = chunks.find({chunkX, chunkZ});
        if (it == chunks.end()) {
            return false; // 区块不存在，视为空气
        }
        
        Chunk* chunk = it->second;
        
        // 检查坐标是否在区块范围内
        if (localX < 0 || localX >= 16 || 
            y < 0 || y >= 16 || 
            localZ < 0 || localZ >= 16) {
            return false;
        }
        
        // 返回该方块是否为实心
        return chunk->m_blocks[localX][y][localZ] != BLOCK_AIR;
    }
    
    // 检测玩家与世界的碰撞
    bool checkCollision(glm::vec3 newPos, std::map<std::pair<int, int>, Chunk*>& chunks) {
        AABB playerBox(newPos, WIDTH, HEIGHT, DEPTH);
        
        // 检查玩家包围盒与周围方块的碰撞
        int minX = (int)floor(playerBox.min.x);
        int maxX = (int)ceil(playerBox.max.x);
        int minY = (int)floor(playerBox.min.y);
        int maxY = (int)ceil(playerBox.max.y);
        int minZ = (int)floor(playerBox.min.z);
        int maxZ = (int)ceil(playerBox.max.z);
        
        for (int x = minX; x < maxX; x++) {
            for (int y = minY; y < maxY; y++) {
                for (int z = minZ; z < maxZ; z++) {
                    if (isBlockSolid(x, y, z, chunks)) {
                        // 方块的AABB
                        AABB blockBox;
                        blockBox.min = glm::vec3(x, y, z);
                        blockBox.max = glm::vec3(x + 1, y + 1, z + 1);
                        
                        if (playerBox.intersects(blockBox)) {
                            return true; // 发生碰撞
                        }
                    }
                }
            }
        }
        
        return false; // 没有碰撞
    }
    
    // 物理更新
    void update(float deltaTime, std::map<std::pair<int, int>, Chunk*>& chunks) {
        // 应用重力
        velocity.y += GRAVITY * deltaTime;
        
        // 限制最大下落速度
        if (velocity.y < TERMINAL_VELOCITY) {
            velocity.y = TERMINAL_VELOCITY;
        }
        
        // 保存旧位置
        glm::vec3 oldPosition = position;
        
        // 分别处理每个轴的移动（避免卡在墙角）
        // X轴移动
        glm::vec3 newPos = position;
        newPos.x += velocity.x * deltaTime;
        if (!checkCollision(newPos, chunks)) {
            position.x = newPos.x;
        } else {
            velocity.x = 0;
        }
        
        // Y轴移动（重力）
        newPos = position;
        newPos.y += velocity.y * deltaTime;
        if (!checkCollision(newPos, chunks)) {
            position.y = newPos.y;
            onGround = false;
        } else {
            if (velocity.y < 0) {
                onGround = true; // 落在地面上
            }
            velocity.y = 0;
        }
        
        // Z轴移动
        newPos = position;
        newPos.z += velocity.z * deltaTime;
        if (!checkCollision(newPos, chunks)) {
            position.z = newPos.z;
        } else {
            velocity.z = 0;
        }
    }
    
    // 移动玩家（水平方向）
    void move(glm::vec3 direction, float speed) {
        // 只设置水平速度，不影响Y轴
        velocity.x = direction.x * speed;
        velocity.z = direction.z * speed;
    }
    
    // 跳跃
    void jump() {
        if (onGround) {
            velocity.y = JUMP_STRENGTH;
            onGround = false;
        }
    }
};

#endif
