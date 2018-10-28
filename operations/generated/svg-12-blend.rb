#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

copyright = '
/* !!!! AUTOGENERATED FILE generated by svg-12-blend.rb !!!!!
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
 */'

a = [
 #     ['multiply',      'cA * cB +  cA * (1 - aB) + cB * (1 - aA)', '5910165f5e64ac11b4f57520e82c99e8'],
      ['screen',        'cA + cB - cA * cB', 'dd4057b688a2721774557d5637e59a50'],
      ['darken',        'MIN (cA * aB, cB * aA) + cA * (1 - aB) + cB * (1 - aA)', '78d5adc0553b920894c5ffc109769a88'],
      ['lighten',       'MAX (cA * aB, cB * aA) + cA * (1 - aB) + cB * (1 - aA)', '81d848688367b57294d91ea1e49880c7'],
      ['difference',    'cA + cB - 2 * (MIN (cA * aB, cB * aA))', 'd49524773e0a1119d9128e3d9799bffc'],
      ['exclusion',     '(cA * aB + cB * aA - 2 * cA * cB) + cA * (1 - aB) + cB * (1 - aA)', 'd26e0029c2f19b8f357069704eca580a']
    ]

b = [
      ['overlay',       '2 * cB > aB',
                        '2 * cA * cB + cA * (1 - aB) + cB * (1 - aA)',
                        'aA * aB - 2 * (aB - cB) * (aA - cA) + cA * (1 - aB) + cB * (1 - aA)',
                        '5b0a600e815dd027ee127cc7c01c26cb'],
      ['color_dodge',   'cA * aB + cB * aA >= aA * aB',
                        'aA * aB + cA * (1 - aB) + cB * (1 - aA)',
                        '(cA == aA ? 1 : cB * aA / (aA == 0 ? 1 : 1 - cA / aA)) + cA * (1 - aB) + cB * (1 - aA)',
                        '30a75546688fe4a12e7d0721b5c357ce'],

      ['color_burn',    'cA * aB + cB * aA <= aA * aB',
                        'cA * (1 - aB) + cB * (1 - aA)',
                        '(cA == 0 ? 1 : (aA * (cA * aB + cB * aA - aA * aB) / cA) + cA * (1 - aB) + cB * (1 - aA))',
                        'e9f58c8e3b67d0ed84d1e1875c414159'],
      ['hard_light',    '2 * cA < aA',
                        '2 * cA * cB + cA * (1 - aB) + cB * (1 - aA)',
                        'aA * aB - 2 * (aB - cB) * (aA - cA) + cA * (1 - aB) + cB * (1 - aA)',
                        '77f3994f122ac63313b0c67ebfddbcfe']
    ]

c = [
      ['soft_light',    '2 * cA < aA',
                        'cB * (aA - (aB == 0 ? 1 : 1 - cB / aB) * (2 * cA - aA)) + cA * (1 - aB) + cB * (1 - aA)',
                        '8 * cB <= aB',
                        'cB * (aA - (aB == 0 ? 1 : 1 - cB / aB) * (2 * cA - aA) * (aB == 0 ? 3 : 3 - 8 * cB / aB)) + cA * (1 - aB) + cB * (1 - aA)',
                        '(aA * cB + (aB == 0 ? 0 : sqrt (cB / aB) * aB - cB) * (2 * cA - aA)) + cA * (1 - aB) + cB * (1 - aA)',
                        '21a00cc30644e8caafe2d42d77b089ef']
    ]

d = [
      ['plus',          'cA + cB',
                        'MIN (aA + aB, 1)',
                        '02568f1753f7ded71ed2534b38f26d73']
    ]

file_head1 = '
#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_boolean (srgb, _("sRGB"), FALSE)
    description (_("Use sRGB gamma instead of linear"))

#else
'

file_head2 = '
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
      babl_model_is (model, "Y\'") ||
      babl_model_is (model, "Y~"))
  {
    format  = babl_format_with_space (use_srgb?"Y~ float":"Y float", space);
  }
  else if (babl_model_is (model, "YA") ||
           babl_model_is (model, "Y\'A") ||
           babl_model_is (model, "Y~A") ||
           babl_model_is (model, "YaA") ||
           babl_model_is (model, "Y\'aA"))
  {
    format  = babl_format_with_space (use_srgb?"Y~aA float":"YaA float", space);
  }
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R\'G\'B\'") ||
           babl_model_is (model, "R~G~B~"))
  {
    format  = babl_format_with_space (use_srgb?"R~G~B~ float":"RGB float", space);
  }
  else if (babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "RGB")     ||
           babl_model_is (model, "R\'G\'B\'A") ||
           babl_model_is (model, "R\'G\'B\'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R\'aG\'aB\'aA"))
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

/* Fast paths */
static gboolean operation_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level)
{
  GeglOperationClass  *operation_class;
  gpointer input, aux;
  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  /* get the raw values this does not increase the reference count */
  input = gegl_operation_context_get_object (context, "input");
  aux = gegl_operation_context_get_object (context, "aux");

  /* pass the input/aux buffers directly through if they are alone*/
  {
    const GeglRectangle *in_extent = NULL;
    const GeglRectangle *aux_extent = NULL;

    if (input)
      in_extent = gegl_buffer_get_abyss (input);

    if ((!input ||
        (aux && !gegl_rectangle_intersect (NULL, in_extent, result))))
      {
         gegl_operation_context_take_object (context, "output",
                                             g_object_ref (aux));
         return TRUE;
      }
    if (aux)
      aux_extent = gegl_buffer_get_abyss (aux);

    if (!aux ||
        (input && !gegl_rectangle_intersect (NULL, aux_extent, result)))
      {
        gegl_operation_context_take_object (context, "output",
                                            g_object_ref (input));
        return TRUE;
      }
  }
  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result, level);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  const Babl *format = gegl_operation_get_format (op, "output");
  gint    components = babl_format_get_n_components (format);
  gint    alpha      = babl_format_has_alpha (format);
  gfloat * GEGL_ALIGNED in = in_buf;
  gfloat * GEGL_ALIGNED aux = aux_buf;
  gfloat * GEGL_ALIGNED out = out_buf;
  gint    i;

  if(aux == NULL)
     return TRUE;
'

file_tail1 = '
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
  operation_class->process      = operation_process;
  operation_class->prepare      = prepare;
'

file_tail2 = '  gegl_operation_class_set_key (operation_class, "categories", "compositors:svgfilter");
}

#endif
'

a.each do
    |item|

    name     = item[0] + ''
    name.gsub!(/_/, '-')
    # hack to avoid a conflict with multiply op generated by math.rb
    compat_name = (name == 'multiply' ? 'svg-' : '') + name
    filename = compat_name + '.c'

    puts "generating #{filename}"
    file = File.open(filename, 'w')

    capitalized = name.capitalize
    swapcased   = name.swapcase
    formula1    = item[1]

    file.write copyright
    file.write file_head1
    file.write "
#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         #{item[0]}
#define GEGL_OP_C_FILE       \"#{filename}\"

#include \"gegl-op.h\"
"
    file.write file_head2
    file.write "
  for (i = 0; i < n_pixels; i++)
    {
      gfloat aA, aB, aD;
      gint   j;

      if (alpha)
      {
        aB = in[components-1];
        aA = aux[components-1];
      }
      else
      {
        aB = aA = 1.0f;
      }
      aD = aA + aB - aA * aB;

      for (j = 0; j < components-alpha; j++)
        {
          gfloat cA, cB;

          cB = in[j];
          cA = aux[j];
          out[j] = CLAMP (#{formula1}, 0, aD);
        }
      if (alpha)
        out[components-1] = aD;
      in  += components;
      aux += components;
      out += components;
    }
"
  file.write file_tail1
  file.write "
  gegl_operation_class_set_keys (operation_class,
  \"name\"        , \"svg:#{name}\",
  \"compat-name\" , \"gegl:#{compat_name}\",
  \"reference-hash\" , \"#{item[2]}\",
  \"description\" ,
        _(\"SVG blend operation #{name} (<code>d = #{formula1}</code>)\"),
        NULL);
"
  file.write file_tail2
  file.close
end

b.each do
    |item|

    name     = item[0] + ''
    name.gsub!(/_/, '-')
    filename = name + '.c'

    puts "generating #{filename}"
    file = File.open(filename, 'w')

    capitalized = name.capitalize
    swapcased   = name.swapcase
    cond1       = item[1]
    formula1    = item[2]
    formula2    = item[3]

    file.write copyright
    file.write file_head1
    file.write "
#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         #{item[0]}
#define GEGL_OP_C_FILE       \"#{filename}\"

#include \"gegl-op.h\"
"
    file.write file_head2
    file.write "
  for (i = 0; i < n_pixels; i++)
    {
      gfloat aA, aB, aD;
      gint   j;

      if (alpha)
      {
        aB = in[components-1];
        aA = aux[components-1];
      }
      else
      {
        aB = aA = 1.0f;
      }
      aD = aA + aB - aA * aB;

      for (j = 0; j < components-alpha; j++)
        {
          gfloat cA, cB;

          cB = in[j];
          cA = aux[j];
          if (#{cond1})
            out[j] = CLAMP (#{formula1}, 0, aD);
          else
            out[j] = CLAMP (#{formula2}, 0, aD);
        }
      if (alpha)
        out[components-1] = aD;
      in  += components;
      aux += components;
      out += components;
    }
"
  file.write file_tail1
  file.write "
  gegl_operation_class_set_keys (operation_class,
  \"name\"        , \"svg:#{name}\",
  \"compat-name\" , \"gegl:#{name}\",
  \"title\"       , \"#{name.capitalize}\",
  \"reference-hash\" , \"#{item[4]}\",
  \"description\" ,
        _(\"SVG blend operation #{name} (<code>if #{cond1}: d = #{formula1} otherwise: d = #{formula2}</code>)\"),
        NULL);
"
  file.write file_tail2
  file.close
end

c.each do
    |item|

    name     = item[0] + ''
    name.gsub!(/_/, '-')
    filename = name + '.c'

    puts "generating #{filename}"
    file = File.open(filename, 'w')

    capitalized = name.capitalize
    swapcased   = name.swapcase
    cond1       = item[1]
    formula1    = item[2]
    cond2       = item[3]
    formula2    = item[4]
    formula3    = item[5]

    file.write copyright
    file.write file_head1
    file.write "
#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         #{item[0]}
#define GEGL_OP_C_FILE       \"#{filename}\"

#include \"gegl-op.h\"
#include <math.h>
"
    file.write file_head2
    file.write "
  for (i = 0; i < n_pixels; i++)
    {
      gfloat aA, aB, aD;
      gint   j;

      if (alpha)
      {
        aB = in[components-1];
        aA = aux[components-1];
      }
      else
      {
        aB = aA = 1.0f;
      }
      aD = aA + aB - aA * aB;

      for (j = 0; j < components-alpha; j++)
        {
          gfloat cA, cB;

          cB = in[j];
          cA = aux[j];
          if (#{cond1})
            out[j] = CLAMP (#{formula1}, 0, aD);
          else if (#{cond2})
            out[j] = CLAMP (#{formula2}, 0, aD);
          else
            out[j] = CLAMP (#{formula3}, 0, aD);
        }
      if (alpha)
      {
        out[components-1] = aD;
      }
      in  += components;
      aux += components;
      out += components;
    }
"
  file.write file_tail1
  file.write "
  gegl_operation_class_set_keys (operation_class,
  \"name\"        , \"gegl:#{name}\",
  \"title\"       , \"#{name.capitalize}\",
  \"reference-hash\" , \"#{item[6]}\",
  \"description\" ,
        _(\"SVG blend operation #{name} (<code>if #{cond1}: d = #{formula1}; if #{cond2}: d = #{formula2}; otherwise: d = #{formula3}</code>)\"),
        NULL);
"
  file.write file_tail2
  file.close
end

d.each do
    |item|

    name     = item[0] + ''
    name.gsub!(/_/, '-')
    filename = name + '.c'

    puts "generating #{filename}"
    file = File.open(filename, 'w')

    capitalized = name.capitalize
    swapcased   = name.swapcase
    formula1    = item[1]
    formula2    = item[2]

    file.write copyright
    file.write file_head1
    file.write "
#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME         #{item[0]}
#define GEGL_OP_C_FILE       \"#{filename}\"

#include \"gegl-op.h\"
"
    file.write file_head2
    file.write "
  for (i = 0; i < n_pixels; i++)
    {
      gfloat aA, aB, aD;
      gint   j;

      if (alpha)
      {
        aB = in[components-1];
        aA = aux[components-1];
      }
      else
      {
        aA = aB = 1.0f;
      }
      aD = #{formula2};

      for (j = 0; j < components-alpha; j++)
        {
          gfloat cA, cB;

          cB = in[j];
          cA = aux[j];
          out[j] = CLAMP (#{formula1}, 0, aD);
        }
      if (alpha)
        out[components-1] = aD;
      in  += components;
      aux += components;
      out += components;
    }
"
  file.write file_tail1
  file.write "

  gegl_operation_class_set_keys (operation_class,
    \"name\"        , \"svg:#{name}\",
    \"title\"       , \"#{name.capitalize}\",
    \"compat-name\" , \"gegl:#{name}\",
    \"reference-hash\" , \"#{item[3]}\",
    \"description\" ,
    _(\"SVG blend operation #{name} (<code>d = #{formula1}</code>)\"),
    NULL);
"
  file.write file_tail2
  file.close
end
