//
// Created by ozzadar on 20/05/23.
//
#pragma once

#pragma once
#include <array>
#include <glm/glm.hpp>
#include <utility>
#include <numbers>
#include <ozz_vulkan/resources/types.h>
#include <spdlog/spdlog.h>

namespace OZZ {

    constexpr double PI = std::numbers::pi;

    inline glm::vec3 getNormal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, bool print = false) {
        glm::vec3 U = p2 - p1;
        glm::vec3 V = p3 - p1;

        auto normal = glm::cross(U, V);
//        glm::vec3.Normal {
//            (U.y * V.z) - (U.z * V.y),
//            (U.z * V.x) - (U.x * V.z),
//            (U.x * V.y) - (U.y * V.x)
//        };

        if (print) {
            spdlog::info("Normal: ({},{},{})", normal.x, normal.y, normal.z);
        }

        return normal;
    }

    constexpr std::array<uint32_t, 36> cubeIndices {
            // Front
            0, 1, 3,
            1, 2, 3,
            // Back
            4, 5, 7,
            5, 6, 7,
            // Left
            8, 9, 11,
            9, 10, 11,
            // Right
            12, 13, 15,
            13, 14, 15,
            // Top
            16, 17, 19,
            17, 18, 19,
            // Bottom
            20, 21, 23,
            21, 22, 23,
    };

    constexpr std::array<uint32_t, 36> invertedCubeIndices {
            // Front
            3, 1, 0,
            3, 2, 1,
            // Back
            7, 5, 4,
            7, 6, 5,
            // Left
            11, 9, 8,
            11, 10, 9,
            // Right
            15, 13, 12,
            15, 14, 13,
            // Top
            19, 17, 16,
            19, 18, 17,
            // Bottom
            23, 21, 20,
            23, 22, 21,
    };

    constexpr std::array<Vertex, 24> cubeVertices {
            // FRONT
            Vertex {
                    .Position = {0.5f, 0.5f, 0.5f},
                    .TexCoord = { 1.f, 1.f },
                    .Normal = { 0, 0, 1.f }
            },
            {
                    .Position = {0.5f, -0.5f, 0.5f},
                    .TexCoord = { 1.f, 0.f },
                    .Normal = { 0, 0, 1.f }
            },
            {
                    .Position = {-0.5f, -0.5f, 0.5f},
                    .TexCoord = { 0.f, 0.f },
                    .Normal = { 0, 0, 1.f }
            },
            {
                    .Position = {-0.5f, 0.5f, 0.5f},
                    .TexCoord = { 0.f, 1.f },
                    .Normal = { 0, 0, 1.f }
            },

            // BACK
            {
                    .Position = {-0.5f, 0.5f, -0.5f},
                    .TexCoord = { 1.f, 1.f },
                    .Normal = { 0, 0, -1.f }
            },
            {
                    .Position = {-0.5f, -0.5f, -0.5f},
                    .TexCoord = { 1.f, 0.f },
                    .Normal = { 0, 0, -1.f }
            },
            {
                    .Position = {0.5f, -0.5f, -0.5f},
                    .TexCoord = { 0.f, 0.f },
                    .Normal = { 0, 0, -1.f }
            },
            {
                    .Position = {0.5f, 0.5f, -0.5f},
                    .TexCoord = { 0.f, 1.f },
                    .Normal = { 0, 0, -1.f }
            },

            // LEFT
            {
                    .Position = {-0.5f, 0.5f, 0.5f},
                    .TexCoord = { 1.f, 1.f },
                    .Normal = { -1.f, 0, 0.f }
            },
            {
                    .Position = {-0.5f, -0.5f, 0.5f},
                    .TexCoord = { 1.f, 0.f },
                    .Normal = { -1.f, 0, 0.f }
            },
            {
                    .Position = {-0.5f, -0.5f, -0.5f},
                    .TexCoord = { 0.f, 0.f },
                    .Normal = { -1.f, 0, 0.f }
            },
            {
                    .Position = {-0.5f, 0.5f, -0.5f},
                    .TexCoord = { 0.f, 1.f },
                    .Normal = { -1.f, 0, 0.f }
            },

            // RIGHT
            {
                    .Position = {0.5f, 0.5f, -0.5f},
                    .TexCoord = { 1.f, 1.f },
                    .Normal = { 1.f, 0, 0.f }
            },
            {
                    .Position = {0.5f, -0.5f, -0.5f},
                    .TexCoord = { 1.f, 0.f },
                    .Normal = { 1.f, 0, 0.f }
            },

            {
                    .Position = {0.5f, -0.5f, 0.5f},
                    .TexCoord = { 0.f, 0.f },
                    .Normal = { 1.f, 0, 0.f }
            },
            {
                    .Position = {0.5f, 0.5f, 0.5f},
                    .TexCoord = { 0.f, 1.f },
                    .Normal = { 1.f, 0, 0.f }
            },

            // TOP
            {
                    .Position = {0.5f, 0.5f, -0.5f},
                    .TexCoord = { 1.f, 1.f },
                    .Normal = { 0.f, 1.f, 0.f }
            },
            {
                    .Position = {0.5f, 0.5f, 0.5f},
                    .TexCoord = { 1.f, 0.f },
                    .Normal = { 0.f, 1.f, 0.f }
            },

            {
                    .Position = {-0.5f, 0.5f, 0.5f},
                    .TexCoord = { 0.f, 0.f },
                    .Normal = { 0.f, 1.f, 0.f }
            },
            {
                    .Position = {-0.5f, 0.5f, -0.5f},
                    .TexCoord = { 0.f, 1.f },
                    .Normal = { 0.f, 1.f, 0.f }
            },

            // BOTTOM
            {
                    .Position = {0.5f, -0.5f, 0.5f},
                    .TexCoord = { 1.f, 1.f },
                    .Normal = { 0.f, -1.f, 0.f }
            },
            {
                    .Position = {0.5f, -0.5f, -0.5f},
                    .TexCoord = { 1.f, 0.f },
                    .Normal = { 0.f, -1.f, 0.f }
            },

            {
                    .Position = {-0.5f, -0.5f, -0.5f},
                    .TexCoord = { 0.f, 0.f },
                    .Normal = { 0.f, -1.f, 0.f }
            },
            {
                    .Position = {-0.5f, -0.5f, 0.5f},
                    .TexCoord = { 0.f, 1.f },
                    .Normal = { 0.f, -1.f, 0.f }
            },
    };


    constexpr std::array<uint32_t, 18> pyramidIndices {
            // Front Tri
            0, 1, 2,
            // Right Tri
            3, 4, 5,
            // Back Tri
            6, 7, 8,
            // Left Tri
            9, 10, 11,
            // Bottom quad
            12, 13, 14,
            13, 15, 14,
    };

    constexpr std::array<Vertex, 16> pyramidVertices {
            // FRONT TRI
            Vertex {
                    .Position = {-0.5f, 0.f, 0.5f},
                    .TexCoord = {0.f, 0.f},
                    .Normal = {0, 0.5, 1.f}
            },
            {
                    .Position = {0.f, 1.f, 0.f},
                    .TexCoord = {0.5f, 1.f},
                    .Normal = {0, 0.5, 1.f}
            },
            {
                    .Position = {0.5f, 0.f, 0.5f},
                    .TexCoord = {1.f, 0.f},
                    .Normal = {0, 0.5, 1.f}
            },
            // RIGHT TRI
            {
                    .Position = {0.5f, 0.f, 0.5f},
                    .TexCoord = {0.f, 0.f},
                    .Normal = {1.f, 0.5f, 0.f}
            },
            {
                    .Position = {0.f, 1.f, 0.f},
                    .TexCoord = {0.5f, 1.f},
                    .Normal = {1.f, 0.5f, 0.f}
            },
            {
                    .Position = {0.5f, 0.f, -0.5f},
                    .TexCoord = {1.f, 0.f},
                    .Normal = {1.f, 0.5f, 0.f}
            },
            //BACK TRI
            {
                    .Position = {0.5f, 0.f, -0.5f},
                    .TexCoord = {0.f, 0.f},
                    .Normal = {0, 0.5, -1.f}
            },
            {
                    .Position = {0.f, 1.f, 0.f},
                    .TexCoord = {0.5f, 1.f},
                    .Normal = {0, 0.5, -1.f}
            },
            {
                    .Position = {-0.5f, 0.f, -0.5f},
                    .TexCoord = {1.f, 0.f},
                    .Normal = {0, 0.5, -1.f}
            },
            // LEFT TRI
            {
                    .Position = {-0.5f, 0.f, -0.5f},
                    .TexCoord = {0.f, 0.f},
                    .Normal = {-1.f, 0.5f, 0.f}
            },
            {
                    .Position = {0.f, 1.f, 0.f},
                    .TexCoord = {0.5f, 1.f},
                    .Normal = {-1.f, 0.5f, 0.f}
            },
            {
                    .Position = {-0.5f, 0.f, 0.5f},
                    .TexCoord = {1.f, 0.f},
                    .Normal = {-1.f, 0.5f, 0.f}
            },
            // Bottom square
            {
                    .Position = {-0.5f, 0.f, -0.5f},
                    .TexCoord = {0.f, 0.f},
                    .Normal = {0, -1.f, 0}
            },
            {
                    .Position = {-0.5f, 0.f, 0.5f},
                    .TexCoord = {0.f, 1.f},
                    .Normal = {0, -1.f, 0}
            },
            {
                    .Position = {0.5f, 0.f, -0.5f},
                    .TexCoord = {1.f, 0.f},
                    .Normal = {0, -1.f, 0}
            },
            {
                    .Position = {0.5f, 0.f, 0.5f},
                    .TexCoord = {1.f, 1.f},
                    .Normal = {0, -1.f, 0}
            }
    };

    static std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateSphere(float radius = 1.f, uint32_t sectors = 50, uint32_t stacks = 50) {
        std::vector<Vertex> vertices {};
        std::vector<uint32_t> indices {};
        uint32_t k1, k2;
        float x, y, z, xy;                              // vertex.Position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex.Normal
        float s, t;                                     // vertex texCoord

        float sectorStep = 2 * PI / sectors;
        float stackStep = PI / stacks;
        float sectorAngle, stackAngle;

        for(uint32_t i = 0; i <= stacks; ++i)
        {
            stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
            xy = radius * cosf(stackAngle);             // r * cos(u)
            z = radius * sinf(stackAngle);              // r * sin(u)

            k1 = i * (sectors + 1);     // beginning of current stack
            k2 = k1 + sectors + 1;
            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same.Position and.Normal, but different tex coords
            for(uint32_t j = 0; j <= sectors; ++j, ++k1, ++k2)
            {
                sectorAngle = static_cast<float>(j) * sectorStep;           // starting from 0 to 2pi

                // vertex.Position (x, y, z)
                x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

                //.Normalized vertex.Normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;

                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / static_cast<float>(sectors);
                t = (float)i / static_cast<float>(stacks);

                vertices.push_back(Vertex{
                        .Position = {x, y, z},
                        .TexCoord = {s, t},
                        .Normal = {nx, ny, nz},
                });


            }
        }

        for(uint32_t i = 0; i < stacks; ++i)
        {
            k1 = i * (sectors + 1);     // beginning of current stack
            k2 = k1 + sectors + 1;      // beginning of next stack

            for(uint32_t j = 0; j < sectors; ++j, ++k1, ++k2)
            {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if(i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                }

                // k1+1 => k2 => k2+1
                if(i != (stacks-1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2 + 1);
                    indices.push_back(k2);
                }
            }
        }

        return {vertices, indices};
    }
}