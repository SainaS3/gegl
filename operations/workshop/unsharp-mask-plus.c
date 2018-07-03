/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (std_dev, _("Radius"), 0.55)
    description(_("Expressed as standard deviation, in pixels"))
    value_range (0.0, 300)
    ui_range    (0.0, 40.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")

property_double (scale, _("Amount"), 4.0)
    description(_("Scaling factor for unsharp-mask, the strength of effect"))
    value_range (0.0, 300.0)
    ui_range    (0.0, 10.0)
    ui_gamma    (3.0)

property_double (threshold, _("Threshold"), 0.0)
    value_range (0.0, 1.0)
    ui_range    (0.0, 1.0)
    ui_gamma    (1.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     unsharp_mask_plus
#define GEGL_OP_C_SOURCE unsharp-mask-plus.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output, *add, *multiply, *subtract, *blur, *multiply2, *absolute, *threshold, *aa, *multiply_mask;

  gegl = operation->node;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");
  add      = gegl_node_new_child (gegl, "operation", "gegl:add", NULL);
  multiply = gegl_node_new_child (gegl, "operation", "gegl:multiply", NULL);
  multiply_mask = gegl_node_new_child (gegl, "operation", "gegl:multiply", NULL);
  multiply2 = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 2.0f, NULL);
  subtract = gegl_node_new_child (gegl, "operation", "gegl:subtract", NULL);
  absolute = gegl_node_new_child (gegl, "operation", "gegl:abs", NULL);
  threshold = gegl_node_new_child (gegl, "operation", "gegl:threshold", NULL);
  aa = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", "std-dev-x", 1.0, "std-dev-y", 1.0, NULL);
  blur     = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);

  gegl_node_link_many (input, subtract, multiply_mask, multiply, NULL);
  gegl_node_link (input, blur);
  gegl_node_link_many (multiply, add, output, NULL);

  gegl_node_link_many (subtract, absolute, multiply2, threshold, aa, NULL);
  gegl_node_connect_from (multiply_mask, "aux",  aa,  "output");

  gegl_node_connect_from (subtract, "aux",   blur,  "output");
  gegl_node_connect_from (add,      "aux",   input, "output");

  gegl_operation_meta_redirect (operation, "threshold", threshold, "value");
  gegl_operation_meta_redirect (operation, "scale", multiply, "value");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");

  gegl_operation_meta_watch_nodes (operation, add, multiply, subtract, blur, threshold, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:unsharp-mask-plus",
    "title",       _("Sharpen (Unsharp Mask)"),
    "categories",  "enhance:sharpen",
    "reference-hash", "5f94a8d1b946c82b1f066f50b9648a5a",
    "description", _("Sharpen image, by adding difference to blurred image, a technique for sharpening originally used in darkrooms."),
    NULL);
}

#endif
