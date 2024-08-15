#pragma once

enum class EDirection :uint8
{
    Forward, Right, Back, Left , Up , Down
};

enum class EBlock
{
    Null, Air, Stone, Dirt, Grass
};

enum class EGenerationType
{
        Surface, Underground
};