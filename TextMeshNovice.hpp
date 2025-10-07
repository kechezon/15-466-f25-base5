#pragma once

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include <hb.h>
#include <hb-ft.h>

#include <SDL3/SDL.h>

/************************************************************************************************************
 * The following code in this file is based on the following:
 * - XOR/Circle renderer code provided by Jim McCann 
 * - Freetype Tutorial: https://freetype.org/freetype2/docs/tutorial/step1.html
 * - Harfbuzz Tutorial: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
 ************************************************************************************************************/

 /****************
 * Render things
 ****************/
#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * 0.5)

struct TextMeshNovice { // single line text
    FT_Library library = 0;
    FT_Face face = 0;
    FT_Error ft_error = 0;

    char fontfile[1024];

    /* Create hb-ft font*/
    hb_font_t *hb_font = nullptr;

    /* Create hb-buffer and populate*/
    hb_buffer_t *hb_buffer = nullptr;

    /* Glyph info */
    unsigned int hb_buffer_len = 0;
    hb_glyph_info_t *info = 0;
    hb_glyph_position_t *pos = 0; // returns array of positions. Same arg 2 return and buffer modification disclaim

    const char *mytext = nullptr;
    std::vector< uint8_t > data = {};
    int data_width = 0;
    int data_height = 0;
    bool data_created = false;

    GLuint buffer_for_color_texture_program = 0;
    GLuint tex = 0;
    GLuint vertex_buffer = 0;
    //format for the mesh data:
    struct Vertex {
        glm::vec2 Position;
        glm::u8vec4 Color;
        glm::vec2 TexCoord;

        // Vertex() : Position(0), Color(0), TexCoord(0) {};
    };
    std::vector< Vertex > attribs = {};

    TextMeshNovice(const char *mytext_) : library(0), face(0), ft_error(0), hb_font(nullptr), hb_buffer(nullptr),
                                          info(0), mytext(mytext_), data(0), attribs(0) {
        /*****************************************************************************************
         * string->char* code from:
         * https://stackoverflow.com/questions/13294067/how-to-convert-string-to-char-array-in-c
         * https://en.cppreference.com/w/c/string/byte/strcpy.html
         *****************************************************************************************/
        std::string path_str = data_path("HammersmithOne-Regular.ttf");
        strcpy_s(fontfile, path_str.length() + 1, path_str.c_str());

        /*********************************************************************************
        * Comes from the following tutorials:
        * https://freetype.org/freetype2/docs/tutorial/step1.html
        * https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
        ********************************************************************************/
        // harfbuzz and freetype get data from glyphs
        if ((ft_error = FT_Init_FreeType( &library ))) {
            printf("Failed to initialize library for text %s... (%i)\n", mytext, ft_error);
            abort();
        }
        // Loads a font face (descriptor of a given typeface and style)
        // Now to access that face data with a face object (models information that globally describes a face)
        if ((ft_error = FT_New_Face(library, fontfile, 0, &face))) {
            printf("Failed to make new face for text %s from file %s... (%i)\n", mytext, fontfile, ft_error);
            abort();
        }
        else {
            printf("Got file %s... sometimes...\n", fontfile);
        }

        // face->size lets you get pixel size. This size object models all information needed w.r.t. character sizes
        // Use FT_Set_Char_Size, passing a handle to the face object you have.
        // Width and height are expressed in 1/64th of a point (0 means same as the other). You can also see the pixel size.
        if ((ft_error = FT_Set_Char_Size(face, FONT_SIZE*64, FONT_SIZE*64, 0, 0))) {
            printf("Failed to set character size for text %s... (%i)\n", mytext, ft_error);
            abort();
        }

        /* Create hb-ft font*/
        hb_font = hb_ft_font_create(face, NULL); // uses face to create hb font. second argument is a callback to call when font object is not needed (destructor)
    };

    // NOTE: You'll need to call create_data_vector and set_position after calling this function
    void set_text(const char *mytext_) {
        mytext = mytext_;
        data.clear();
    }

    void create_data_vector() {
        /*************************
         * From Harfbuzz Tutorial
         *************************/
        hb_buffer = hb_buffer_create(); // takes no arguments
        hb_buffer_add_utf8(hb_buffer, mytext, -1, 0, -1);
        hb_buffer_guess_segment_properties(hb_buffer); // sets unset buffer segment properties based on buffer's contents

        /* Shape the buffer! */
        hb_shape(hb_font, hb_buffer, NULL, 0); // arg 3 (features) controls features applied during shaping.
                                                // arg 4 tells you length of features array

        /* Glyph info */
        // hb_buffer_len = hb_buffer_get_length(hb_buffer);
        info = hb_buffer_get_glyph_infos(hb_buffer, &hb_buffer_len); // arg 2 is pointer to an unsigned int to write length of output array to
                                                                            // if you modify the buffer contents, the return pointer is invalidated!
                                                                            // Making arg 2 null means I don't care about the length written to the hb_buffer
        pos = hb_buffer_get_glyph_positions(hb_buffer, NULL); // returns array of positions. Same arg 2 return and buffer modification disclaimer
        /*************************
         * /end Harfbuzz Tutorial
         *************************/

        /***************************
         * Based on Jim McCann's provided
         * XOR/Circle Example
         ***************************/
        //texture size:
        data_width = 0;
        data_height = 0;
        unsigned int glyph_up = 0;
        unsigned int glyph_down = 0;
        //pixel data for texture:
        FT_GlyphSlot slot = face->glyph; // shortcut to where we'll draw the glyph. FT_GlyphSlot is a pointer type

        // Data of the message
        for (unsigned int i = 0; i < hb_buffer_len; i++) { // get GLsizei length (assume it's on one line)
            FT_UInt	glyph_index = FT_Get_Char_Index(face, mytext[i]);
            FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

            // data_width += (GLsizei)(slot->bitmap_left + (pos[i].x_advance >> 6));
            data_width += (GLsizei)((pos[i].x_advance >> 6));
		    if (slot->bitmap_left < 0) data_width += std::abs(slot->bitmap_left);
            glyph_up = (GLsizei)(std::max(glyph_up, (unsigned int) slot->bitmap_top));
            glyph_down = (GLsizei)(std::max(glyph_down, slot->bitmap.rows - slot->bitmap_top));
        }
        data_height = glyph_up + glyph_down;
        data = std::vector<uint8_t> (data_width*data_height, 0);

        unsigned int cursor_x = 0;
        unsigned int cursor_y = 0;
        for (unsigned int i = 0; i < hb_buffer_len; i++) {
            FT_UInt	glyph_index = FT_Get_Char_Index(face, mytext[i]);
            FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

            // loop things to draw the character
            for (unsigned int y = 0; y < (unsigned int)(slot->bitmap.rows); y++) {
                unsigned int glyph_width = slot->bitmap.width;
                int my_down = slot->bitmap.rows - slot->bitmap_top; // how far below the baseline do you dip

                // we read from the bitmap and to the data in opposite directions, so the indexing needs to be flipped!
                for (unsigned int x = 0; x < glyph_width; x++) {
                    uint8_t c = (slot->bitmap.buffer)[(slot->bitmap.rows - y - 1) * glyph_width + x]; // retrieve value
                    data[(cursor_y + y + glyph_down - my_down) * data_width + (cursor_x + x)] = c; // place in data to be drawn
                }
                //printf("\n");
            }
            // printf("\n\n");
            cursor_x += pos[i].x_advance >> 6;
        }

        /********************************
         * /end Jim McCann-inspired Code
         ********************************/
        data_created = true;
    }

    /*****************************************
     * The next four functions are based on
     * Jim McCann's XOR/Circle Code
     *****************************************/
    void create_mesh(SDL_Window *window, float clip_center_x, float clip_center_y, float clip_height,
                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        //---------------  create and upload texture data ----------------

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // This code, up to "draw mesh" (exclusive) should be called only when creating new text
        // need a name for the texture object:
        glGenTextures(1, &tex); // store 1 texture name in tex1
        //attach texture object to the GL_TEXTURE_2D binding point:
        glBindTexture(GL_TEXTURE_2D, tex);

        //upload data: (see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml )
        glTexImage2D(
            GL_TEXTURE_2D, //target -- the binding point this call is uploading to
            0, //level -- the mip level; 0 = the base level
            GL_R8,  //internalformat -- channels and storage used on the GPU for the texture; GL_R8 means one channel, 8-bit fixed point
            data_width, //width of texture
            data_height, //height of texture
            0, //border -- must be 0
            GL_RED, //format -- how the data to be uploaded is structured; GL_RED means one channel
            GL_UNSIGNED_BYTE, //type -- how each element of a pixel is stored
            data.data() //data -- pointer to the texture data
        );
        //set up texture sampling state:
        //clamp texture coordinate to edge of texture: (GL_REPEAT can also be useful in some cases)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //use linear interpolation to magnify:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //use trilinear interpolation (linear interpolation of linearly-interpolated mipmaps) to minify:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        //ask OpenGL to make the mipmaps for us:
        glGenerateMipmap(GL_TEXTURE_2D);

        //de-attach texture:
        glBindTexture(GL_TEXTURE_2D, 0);

        //----------- set up place to store mesh that references the data -----------

        //create a buffer object to store mesh data in:
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //(buffer created when bound)
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //create a vertex array object that references the buffer:
        buffer_for_color_texture_program = 0;
        glGenVertexArrays(1, &buffer_for_color_texture_program);
        glBindVertexArray(buffer_for_color_texture_program);

        //configure the vertex array object:

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //will take data from 'buffer'

        //set up Position to read from the buffer:
        //see https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
        glVertexAttribPointer(
            color_texture_program->Position_vec4, //attribute
            2, //size
            GL_FLOAT, //type
            GL_FALSE, //normalized
            sizeof(Vertex), //stride
            (GLbyte *)0 + offsetof(Vertex, Position) //offset
        );
        glEnableVertexAttribArray(color_texture_program->Position_vec4);

        //set up Color to read from the buffer:
        glVertexAttribPointer( color_texture_program->Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, Color));
        glEnableVertexAttribArray(color_texture_program->Color_vec4);

        //set up TexCoord to read from the buffer:
        glVertexAttribPointer( color_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, TexCoord));
        glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

        //done configuring vertex array, so unbind things:
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);


        //----------- create and upload a mesh that references the data -----------

        attribs.reserve(4);
        //if drawn as a triangle strip, this will be a square with the lower-left corner at (0,0) and the upper right at (1,1):
        
        set_position(window, clip_center_x, clip_center_y, clip_height, r, g, b, a);
    };

    void set_position(SDL_Window *window, float clip_center_x, float clip_center_y, float clip_height,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        // get ratio
        if (data_height == 0) {
            printf("Glyphs of string %s all had height of 0! Don't try to print just an empty character, please.\n", mytext);
            abort();
        }

        assert(data_height != 0);

        int window_w, window_h;

        if (!SDL_GetWindowSize(window, &window_w, &window_h)) {
            printf("Failed to get window size?!\n");
            abort();
        }
        
        float clip_width = ((float)window_h / window_w) * clip_height * data_width / data_height;
        float left_clip = clip_center_x - (clip_width / 2); 
        float right_clip = clip_center_x + (clip_width / 2); 
        float top_clip = clip_center_y + (clip_height / 2); 
        float bottom_clip = clip_center_y - (clip_height / 2); 
        
        attribs.clear();
        attribs.reserve(4);

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(left_clip, bottom_clip),
            .Color = glm::u8vec4(r, g, b, a),
            .TexCoord = glm::vec2(0.0f, 0.0f),
        });

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(left_clip, top_clip),
            .Color = glm::u8vec4(r, g, b, a),
            .TexCoord = glm::vec2(0.0f, 1.0f),
        });

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(right_clip, bottom_clip),
            .Color = glm::u8vec4(r, g, b, a),
            .TexCoord = glm::vec2(1.0f, 0.0f),
        });

        attribs.emplace_back(Vertex {
            .Position = glm::vec2(right_clip, top_clip),
            .Color = glm::u8vec4(r, g, b, a),
            .TexCoord = glm::vec2(1.0f, 1.0f),
        });

        //upload attribs to buffer:
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, attribs.size() * sizeof((attribs)[0]), attribs.data(), GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_text_mesh() {
        glUseProgram(color_texture_program->program);
		//draw with attributes from our buffer, as referenced by the vertex array:
		glBindVertexArray(buffer_for_color_texture_program);
		//draw using texture stored in tex:
		glBindTexture(GL_TEXTURE_2D, tex);
		
		//this particular shader program multiplies all positions by this matrix: (hmm, old naming style; I should have fixed that)
		// (just setting it to the identity, so Positions are directly in clip space)
		glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

		//draw without depth testing (so will draw atop everything else):
		glDisable(GL_DEPTH_TEST);
		//draw with alpha blending (so transparent parts of the texture look transparent):
		glEnable(GL_BLEND);
		//standard 'over' blending:
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//actually draw:
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)attribs.size());

		//turn off blending:
		glDisable(GL_BLEND);
		//...leave depth test off, since code that wants it will turn it back on

		//unbind texture, vertex array, program:
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
    }

    ~TextMeshNovice() {
        // ----------- free allocated buffers / data -----------
        glDeleteVertexArrays(1, &buffer_for_color_texture_program);
        buffer_for_color_texture_program = 0;
        glDeleteBuffers(1, &vertex_buffer);
        vertex_buffer = 0;
        glDeleteTextures(1, &tex);
        tex = 0;
    }
};