/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "graphics/math-defs.h"
#include "obs-scene.h"

static inline void signal_item_destroy(struct obs_scene_item *item,
		struct calldata *params)
{
	calldata_setptr(params, "scene", item->parent);
	calldata_setptr(params, "item", item);
	signal_handler_signal(item->parent->source->signals, "remove",
			params);
	calldata_clear(params);
}

static const char *scene_getname(const char *locale)
{
	/* TODO: locale lookup of display name */
	return "Scene";
}

static void *scene_create(const char *settings, struct obs_source *source)
{
	struct obs_scene *scene = bmalloc(sizeof(struct obs_scene));
	scene->source     = source;
	scene->first_item = NULL;

	if (pthread_mutex_init(&scene->mutex, NULL) != 0) {
		blog(LOG_ERROR, "scene_create: Couldn't initialize mutex");
		bfree(scene);
		return NULL;
	}

	return scene;
}

static void scene_destroy(void *data)
{
	struct obs_scene *scene = data;
	struct obs_scene_item *item = scene->first_item;
	struct calldata params = {0};

	while (item) {
		struct obs_scene_item *del_item = item;
		item = item->next;

		signal_item_destroy(del_item, &params);

		if (del_item->source)
			obs_source_release(del_item->source);
		bfree(del_item);
	}

	calldata_free(&params);
	pthread_mutex_destroy(&scene->mutex);
	bfree(scene);
}

static uint32_t scene_get_output_flags(void *data)
{
	return SOURCE_VIDEO;
}

static inline void detach_sceneitem(struct obs_scene_item *item)
{
	if (item->prev)
		item->prev->next = item->next;
	else
		item->parent->first_item = item->next;

	if (item->next)
		item->next->prev = item->prev;
}

static inline void attach_sceneitem(struct obs_scene_item *item,
		struct obs_scene_item *prev)
{
	item->prev = prev;

	if (prev) {
		item->next = prev->next;
		if (prev->next)
			prev->next->prev = item;
		prev->next = item;
	} else {
		item->next = item->parent->first_item;
		item->parent->first_item = item;
	}
}

static void scene_video_render(void *data)
{
	struct obs_scene *scene = data;
	struct obs_scene_item *item = scene->first_item;

	pthread_mutex_lock(&scene->mutex);

	while (item) {
		if (obs_source_removed(item->source)) {
			struct obs_scene_item *del_item = item;
			item = item->next;

			obs_sceneitem_destroy(del_item);
			continue;
		}

		gs_matrix_push();
		gs_matrix_translate3f(item->origin.x, item->origin.y, 0.0f);
		gs_matrix_scale3f(item->scale.x, item->scale.y, 1.0f);
		gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, RAD(-item->rot));
		gs_matrix_translate3f(-item->pos.x, -item->pos.y, 0.0f);

		obs_source_video_render(item->source);

		gs_matrix_pop();

		item = item->next;
	}

	pthread_mutex_unlock(&scene->mutex);
}

static uint32_t scene_getsize(void *data)
{
	return 0;
}

static const struct source_info scene_info =
{
	.id               = "scene",
	.getname          = scene_getname,
	.create           = scene_create,
	.destroy          = scene_destroy,
	.get_output_flags = scene_get_output_flags,
	.video_render     = scene_video_render,
	.getwidth         = scene_getsize,
	.getheight        = scene_getsize,
};

obs_scene_t obs_scene_create(const char *name)
{
	struct obs_source *source = bmalloc(sizeof(struct obs_source));
	struct obs_scene  *scene;

	memset(source, 0, sizeof(struct obs_source));
	if (!obs_source_init_handlers(source)) {
		bfree(source);
		return NULL;
	}

	scene = scene_create(NULL, source);
	source->data = scene;

	assert(scene);
	if (!scene) {
		bfree(source);
		return NULL;
	}

	source->name  = bstrdup(name);
	source->type  = SOURCE_SCENE;

	scene->source = source;
	obs_source_init(source, NULL, &scene_info);
	memcpy(&source->callbacks, &scene_info, sizeof(struct source_info));
	return scene;
}

int obs_scene_addref(obs_scene_t scene)
{
	return obs_source_addref(scene->source);
}

int obs_scene_release(obs_scene_t scene)
{
	if (scene)
		return obs_source_release(scene->source);
	return 0;
}

obs_source_t obs_scene_getsource(obs_scene_t scene)
{
	return scene->source;
}

obs_scene_t obs_scene_fromsource(obs_source_t source)
{
	if (source->type != SOURCE_SCENE)
		return NULL;

	return source->data;
}

obs_sceneitem_t obs_scene_findsource(obs_scene_t scene, const char *name)
{
	struct obs_scene_item *item;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;
	while (item) {
		if (strcmp(item->source->name, name) == 0) {
			break;
		}

		item = item->next;
	}

	pthread_mutex_unlock(&scene->mutex);

	return item;
}

void obs_scene_enum_items(obs_scene_t scene,
		bool (*callback)(obs_scene_t, obs_sceneitem_t, void*),
		void *param)
{
	struct obs_scene_item *item;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;
	while (item) {
		if (!callback(scene, item, param))
			break;

		item = item->next;
	}

	pthread_mutex_unlock(&scene->mutex);
}

obs_sceneitem_t obs_scene_add(obs_scene_t scene, obs_source_t source)
{
	struct obs_scene_item *last;
	struct obs_scene_item *item = bmalloc(sizeof(struct obs_scene_item));
	struct calldata params = {0};

	memset(item, 0, sizeof(struct obs_scene_item));
	item->source  = source;
	item->visible = true;
	item->parent  = scene;
	vec2_set(&item->scale, 1.0f, 1.0f);

	if (source)
		obs_source_addref(source);

	pthread_mutex_lock(&scene->mutex);

	last = scene->first_item;
	if (!last) {
		scene->first_item = item;
	} else {
		while (last->next)
			last = last->next;

		last->next = item;
		item->prev = last;
	}

	pthread_mutex_unlock(&scene->mutex);

	calldata_setptr(&params, "scene", scene);
	calldata_setptr(&params, "item", item);
	signal_handler_signal(scene->source->signals, "add", &params);
	calldata_free(&params);

	return item;
}

int obs_sceneitem_destroy(obs_sceneitem_t item)
{
	int ref = 0;

	if (item) {
		struct calldata params = {0};
		signal_item_destroy(item, &params);
		calldata_free(&params);

		pthread_mutex_lock(&item->parent->mutex);
		detach_sceneitem(item);
		pthread_mutex_unlock(&item->parent->mutex);

		if (item->source)
			ref = obs_source_release(item->source);
		bfree(item);
	}

	return ref;
}

obs_scene_t obs_sceneitem_getscene(obs_sceneitem_t item)
{
	return item->parent;
}

obs_source_t obs_sceneitem_getsource(obs_sceneitem_t item)
{
	return item->source;
}

void obs_sceneitem_setpos(obs_sceneitem_t item, const struct vec2 *pos)
{
	vec2_copy(&item->pos, pos);
}

void obs_sceneitem_setrot(obs_sceneitem_t item, float rot)
{
	item->rot = rot;
}

void obs_sceneitem_setorigin(obs_sceneitem_t item, const struct vec2 *origin)
{
	vec2_copy(&item->origin, origin);
}

void obs_sceneitem_setscale(obs_sceneitem_t item, const struct vec2 *scale)
{
	vec2_copy(&item->scale, scale);
}

void obs_sceneitem_setorder(obs_sceneitem_t item, enum order_movement movement)
{
	struct obs_scene *scene = item->parent;

	pthread_mutex_lock(&scene->mutex);

	detach_sceneitem(item);

	if (movement == ORDER_MOVE_UP) {
		attach_sceneitem(item, item->prev);

	} else if (movement == ORDER_MOVE_DOWN) {
		attach_sceneitem(item, item->next);

	} else if (movement == ORDER_MOVE_TOP) {
		struct obs_scene_item *last = item->next;
		if (!last) {
			last = item->prev;
		} else {
			while (last->next)
				last = last->next;
		}

		attach_sceneitem(item, last);

	} else if (movement == ORDER_MOVE_BOTTOM) {
		attach_sceneitem(item, NULL);
	}

	pthread_mutex_unlock(&scene->mutex);
}

void obs_sceneitem_getpos(obs_sceneitem_t item, struct vec2 *pos)
{
	vec2_copy(pos, &item->pos);
}

float obs_sceneitem_getrot(obs_sceneitem_t item)
{
	return item->rot;
}

void obs_sceneitem_getorigin(obs_sceneitem_t item, struct vec2 *origin)
{
	vec2_copy(origin, &item->origin);
}

void obs_sceneitem_getscale(obs_sceneitem_t item, struct vec2 *scale)
{
	vec2_copy(scale, &item->scale);
}
