
/* !!!! AUTOGENERATED FILE generated by svg-12-porter-duff.rb !!!!!
 *
 * This file is an image processing operation for GEGL
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
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 *  Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 *            2007 John Marshall
 *            2013 Daniel Sabo
 *
 * SVG rendering modes; see:
 *     http://www.w3.org/TR/SVG12/rendering.html
 *     http://www.w3.org/TR/2004/WD-SVG12-20041027/rendering.html#comp-op-prop
 *
 *     aA = aux(src) alpha      aB = in(dst) alpha      aD = out alpha
 *     cA = aux(src) colour     cB = in(dst) colour     cD = out colour
 *
 * !!!! AUTOGENERATED FILE !!!!!
 */
#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_boolean (srgb, _("sRGB"), FALSE)
    description (_("Use sRGB gamma instead of linear"))

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         dst_out
#define GEGL_OP_C_FILE        "dst-out.c"

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  int use_srgb = GEGL_PROPERTIES (operation)->srgb?1:0;
  const Babl *format = gegl_operation_get_source_format (operation, "input");
  const Babl *space = NULL;
  const Babl *model = NULL;
  if (!format)
    format = gegl_operation_get_source_format (operation, "aux");
  if (format)
  {
    model = babl_format_get_model (format);
  }

  if (babl_model_is (model, "Y") ||
      babl_model_is (model, "Y'") ||
      babl_model_is (model, "Y~"))
  {
    format  = babl_format_with_space (use_srgb?"Y~ float":"Y float", space);
  }
  else if (babl_model_is (model, "YA") ||
           babl_model_is (model, "Y'A") ||
           babl_model_is (model, "Y~A") ||
           babl_model_is (model, "YaA") ||
           babl_model_is (model, "Y'aA"))
  {
    format  = babl_format_with_space (use_srgb?"Y~aA float":"YaA float", space);
  }
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R'G'B'") ||
           babl_model_is (model, "R~G~B~"))
  {
    format  = babl_format_with_space (use_srgb?"R~G~B~ float":"RGB float", space);
  }
  else if (babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "RGB")     ||
           babl_model_is (model, "R'G'B'A") ||
           babl_model_is (model, "R'G'B'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R'aG'aB'aA"))
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~a float":"RaGaBaA float", space);
  }
  else
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~a float":"RaGaBaA float", space);
  }

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation        *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gint i;
  gfloat * GEGL_ALIGNED in = in_buf;
  gfloat * GEGL_ALIGNED aux = aux_buf;
  gfloat * GEGL_ALIGNED out = out_buf;
  const Babl *format = gegl_operation_get_format (op, "output");
  gint    components = babl_format_get_n_components (format);
  gint    alpha      = babl_format_has_alpha (format);

  if (!aux)
    {
      for (i = 0; i < n_pixels; i++)
        {
          gint   j;
          gfloat aA G_GNUC_UNUSED, aB G_GNUC_UNUSED, aD G_GNUC_UNUSED;

          aB = alpha?in[components-alpha]:1.0f;
          aA = 0.0f;
          aD = aB * (1.0f - aA);

          for (j = 0; j < components-alpha; j++)
            {
              gfloat cA G_GNUC_UNUSED, cB G_GNUC_UNUSED;

              cB = in[j];
              cA = 0.0f;
              out[j] = cB * (1.0f - aA);
            }
          if (alpha)
            out[components-1] = aD;
          in  += components;
          out += components;
        }
    }
  else
    {
      for (i = 0; i < n_pixels; i++)
        {
          gint   j;
          gfloat aA G_GNUC_UNUSED, aB G_GNUC_UNUSED, aD G_GNUC_UNUSED;

          if (alpha)
          {
            aB = in[components-1];
            aA = aux[components-1];
          }
          else
          {
            aB = aA = 1.0f;
          }
          aD = aB * (1.0f - aA);

          for (j = 0; j < components-alpha; j++)
            {
              gfloat cA G_GNUC_UNUSED, cB G_GNUC_UNUSED;

              cB = in[j];
              cA = aux[j];
              out[j] = cB * (1.0f - aA);
            }
          if (alpha)
            out[components-alpha] = aD;
          in  += components;
          aux += components;
          out += components;
        }
    }
  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare = prepare;


  gegl_operation_class_set_keys (operation_class,
    "name"       , "svg:dst-out",
    "compat-name", "gegl:dst-out",
    "title"      , "Dst-out",
    "reference-hash" , "b0ffe0c9b9a5a48d21df751ce576ffa9",
    "categories" , "compositors:porter-duff",
    "description",
        _("Porter Duff operation dst-out (d = cB * (1.0f - aA))"),
        NULL);
 

}

#endif
