/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-audio.h"

enum
{
  PROP_0,
  PROP_STRING
};

struct _GeglAudioPrivate
{
  int foo;
};

static void      set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec);
static void      get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec);

G_DEFINE_TYPE (GeglAudio, gegl_audio, G_TYPE_OBJECT)

static void
gegl_audio_init (GeglAudio *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), GEGL_TYPE_AUDIO, GeglAudioPrivate);
}

static void
gegl_audio_class_init (GeglAudioClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_STRING,
                                   g_param_spec_string ("string",
                                                        "String",
                                                        "A String representation of the GeglAudio",
                                                        "",
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GeglAudioPrivate));
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  //GeglAudio *audio = GEGL_AUDIO (gobject);

  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  //GeglAudio *audio = GEGL_AUDIO (gobject);

  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

GeglAudio *
gegl_audio_new (void)
{
  void *string = NULL;
  if (string)
    return g_object_new (GEGL_TYPE_AUDIO, "string", string, NULL);

  return g_object_new (GEGL_TYPE_AUDIO, NULL);
}

GeglAudio *
gegl_audio_duplicate (GeglAudio *audio)
{
  GeglAudio *new;

  g_return_val_if_fail (GEGL_IS_AUDIO (audio), NULL);

  new = g_object_new (GEGL_TYPE_AUDIO, NULL);

  memcpy (new->priv, audio->priv, sizeof (GeglAudioPrivate));

  return new;
}

/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglAudio as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_PARAM_AUDIO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PARAM_AUDIO, GeglParamAudio))
#define GEGL_IS_PARAM_AUDIO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_PARAM_AUDIO))

typedef struct _GeglParamAudio GeglParamAudio;

struct _GeglParamAudio
{
  GParamSpec parent_instance;

  GeglAudio *default_audio;
};

static void
gegl_param_audio_init (GParamSpec *self)
{
  GEGL_PARAM_AUDIO (self)->default_audio = NULL;
}

#if 0
GeglAudio *
gegl_param_spec_audio_get_default (GParamSpec *self)
{
  return GEGL_PARAM_AUDIO (self)->default_audio;
}
#endif

static void
gegl_param_audio_finalize (GParamSpec *self)
{
  GeglParamAudio  *param_audio  = GEGL_PARAM_AUDIO (self);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (GEGL_TYPE_PARAM_AUDIO));

  if (param_audio->default_audio)
    {
      g_object_unref (param_audio->default_audio);
      param_audio->default_audio = NULL;
    }

  g_warning ("...\n");

  parent_class->finalize (self);
}

static void
gegl_param_audio_set_default (GParamSpec *param_spec,
                              GValue     *value)
{
  GeglParamAudio *gegl_audio = GEGL_PARAM_AUDIO (param_spec);

  if (gegl_audio->default_audio)
    g_value_take_object (value, gegl_audio_duplicate (gegl_audio->default_audio));
}

GType
gegl_param_audio_get_type (void)
{
  static GType param_audio_type = 0;

  if (G_UNLIKELY (param_audio_type == 0))
    {
      static GParamSpecTypeInfo param_audio_type_info = {
        sizeof (GeglParamAudio),
        0,
        gegl_param_audio_init,
        0,
        gegl_param_audio_finalize,
        gegl_param_audio_set_default,
        NULL,
        NULL
      };
      param_audio_type_info.value_type = GEGL_TYPE_AUDIO;

      param_audio_type = g_param_type_register_static ("GeglParamAudio",
                                                       &param_audio_type_info);
    }

  return param_audio_type;
}

GParamSpec *
gegl_param_spec_audio (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
           //            GeglAudio   *default_audio,
                       GParamFlags  flags)
{
  GeglParamAudio *param_audio;

  param_audio = g_param_spec_internal (GEGL_TYPE_PARAM_AUDIO,
                                       name, nick, blurb, flags);

  //param_audio->default_audio = default_audio;
  //if (default_audio)
  //  g_object_ref (default_audio);

  return G_PARAM_SPEC (param_audio);
}
