#include "renderer.h"
#include <stdlib.h>
#include <assert.h>

#define RENDER_QUEUE_SIZE (4096 * 16)
static RenderCommand render_queue[RENDER_QUEUE_SIZE];
static int32_t queue_index;

#define DEBUG_DRAW_SPRITE_BOX 1

void render_queue_flush();

void render_sprite_animated(SpriteAnimated* sprite, Vector2 position, int32_t layer, uint32_t frame_index, Color tint)
{
    render_queue[queue_index] = (RenderCommand){
        .position = position,
        .layer = layer,
        .tint = tint,
        .sprite.type = SPRITE_TYPE_ANIMATED,
        .original_index = queue_index,
    };
    render_queue[queue_index].sprite.animated = *sprite;
    render_queue[queue_index].sprite.animated.frame_index = frame_index;

    queue_index++;

    if (queue_index >= ARRAY_LENGTH(render_queue))
    {
        assert(false && "Too many render commands");
        render_queue_flush();
    }
}

void render_sprite_static_atlas(SpriteStaticAtlas* atlas, Rectangle src, Vector2 position, int32_t layer, Color tint)
{
    render_queue[queue_index] = (RenderCommand){
        .position = position,
        .layer = layer,
        .tint = tint,
        .sprite.type = SPRITE_TYPE_STATIC_ATLAS,
        .sprite.static_atlas.src_rec = src,
        .original_index = queue_index,
    };
    render_queue[queue_index].sprite.static_atlas = *atlas;
    render_queue[queue_index].sprite.static_atlas.src_rec = src;
    queue_index++;

    if (queue_index >= ARRAY_LENGTH(render_queue))
    {
        assert(false && "Too many render commands");
        render_queue_flush();
    }
}

static void
do_render_sprite_static_atlas(RenderCommand* q)
{
    Rectangle src = q->sprite.static_atlas.src_rec;

    Vector2 scale = q->sprite.static_atlas.scale;

    Vector2 origin = q->sprite.static_atlas.origin;
    Vector2 new_origin = Vector2Add(origin, (Vector2) { (src.width * scale.x) / 2.0f, (src.height * scale.y) / 2.0f });
    Vector2 draw_position = Vector2Subtract(q->position, new_origin);

    Color tint = q->tint;
    tint.a = (uint8_t)(q->sprite.static_atlas.opacity * 255.0f);
    Rectangle dst_rect = (Rectangle){draw_position.x, draw_position.y, src.width * scale.x, src.height * scale.y};
    DrawTexturePro(q->sprite.static_atlas.texture, src, dst_rect, origin, 0, tint);

#if DEBUG_DRAW_SPRITE_BOX
	DrawRectangleLinesEx(dst_rect, 2, RED);
#endif
}

static void
do_render_sprite_animated(RenderCommand* q)
{
	int idx = q->sprite.animated.frame_index;
	int i = idx % q->sprite.animated.atlas.column_count;
	int j = q->sprite.animated.atlas.row_count - 1 - idx / q->sprite.animated.atlas.row_count;
	float w = (float)q->sprite.animated.atlas.piece_width;
	float h = (float)q->sprite.animated.atlas.piece_height;

	Rectangle src = (Rectangle){ i * w, j * h, w, h };
	Rectangle dst = (Rectangle){ q->position.x, q->position.y, w * q->sprite.animated.atlas.scale, h * q->sprite.animated.atlas.scale };

	Vector2 origin = (Vector2){  idx * q->sprite.animated.move_factor.x, idx * q->sprite.animated.move_factor.y };
	origin = Vector2Add(origin, q->sprite.animated.offset);

	// Centered
	if(q->sprite.animated.flags & SPRITE_FLAG_CENTERED)
		origin = Vector2Add(origin, (Vector2) { w / 2.0f, h / 2.0f });

	Color tint = q->tint;
	tint.a = MIN((uint8_t)(q->sprite.animated.opacity * 255.0f), tint.a);
	DrawTexturePro(q->sprite.animated.atlas.texture, src, dst, origin, 0, tint);

#if DEBUG_DRAW_SPRITE_BOX
	Vector2 dpos = q->position;
	if (q->sprite.animated.flags & SPRITE_FLAG_CENTERED)
		dpos = Vector2Subtract(dpos, (Vector2) { w / 2.0f, h / 2.0f });
	DrawRectangleLines((int)dpos.x, (int)dpos.y, src.width, src.height, RED);
	Color dimyellow = ColorAlpha(YELLOW, .2f);
	DrawLine(dpos.x, dpos.y + src.height / 2, dpos.x + src.width, dpos.y + src.height / 2, dimyellow);
	DrawLine(dpos.x + src.width / 2, dpos.y, dpos.x + src.width / 2, dpos.y + src.height, dimyellow);
#endif
}

static int 
compare_render_command(void const* left, void const* right)
{
    RenderCommand* left_comm = (RenderCommand*)left;
    RenderCommand* right_comm = (RenderCommand*)right;

    int result = left_comm->layer - right_comm->layer;
    if(result == 0)
        result = left_comm->original_index - right_comm->original_index;
    return result;
}

void
render_queue_flush()
{
    qsort(render_queue, queue_index, sizeof(RenderCommand), compare_render_command);

    for (uint32_t i = 0; i < queue_index; ++i)
    {
        RenderCommand* command = render_queue + i;

        switch (command->sprite.type)
        {
            default:
            case SPRITE_TYPE_STATIC_ATLAS: do_render_sprite_static_atlas(command);  break;
            case SPRITE_TYPE_ANIMATED:     do_render_sprite_animated(command);      break;
        }
    }

    queue_index = 0;
}

int 
sprite_frame_count(SpriteAnimated* sprite)
{
    int frame_count = sprite->atlas.piece_count;
    return frame_count;
}