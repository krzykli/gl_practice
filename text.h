#ifndef TEXTH
#define TEXTH

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library  library;

unsigned int text_VAO, text_VBO;

struct Character {
    unsigned int textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};


void text_initialize_font(const char* font, Character* text_characters)
{
    u32 error;
    error = FT_Init_FreeType(&library);
    if (error)
    {
        print("Error initializing FreeType");
    }

    FT_Face face;
    error = FT_New_Face(library,
                        font,
                        0,
                        &face );
    if (error)
    {
         print("Failed to initialize font: %s", font);
    }
    /*error = FT_Set_Char_Size(*/
          /*face,    [> handle to face object           <]*/
          /*0,       [> char_width in 1/64th of points  <]*/
          /*16*64,   [> char_height in 1/64th of points <]*/
          /*96,     [> dpi horizontal device resolution    <]*/
          /*96);   [> dpi vertical device resolution      <]*/

    int font_size = 48;
    FT_Set_Pixel_Sizes(face, 0, font_size);  

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    for (unsigned char c = 0; c < 128; ++c)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            print("ERROR::FREETYTPE: Failed to load Glyph");
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        text_characters[byte(c)] = character;
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    glGenVertexArrays(1, &text_VAO);
    glGenBuffers(1, &text_VBO);
}


void text_draw(const char* text, glm::vec3 color, glm::vec2 position, float scale,
               Character* text_characters, glm::mat4 ortho_projection, u32 font_shader_program_id)
{
    // Draw text
    // https://learnopengl.com/In-Practice/Text-Rendering
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(text_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
    // The 2D quad requires 6 vertices of 4 floats each, so we reserve 6 *
    // 4 floats of memory. Because we'll be updating the content of the
    // VBO's memory quite often we'll allocate the memory with
    // GL_DYNAMIC_DRAW
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(font_shader_program_id);
    glUniform3f(glGetUniformLocation(font_shader_program_id, "textColor"), color.x, color.y, color.z);
    glUniformMatrix4fv(
        glGetUniformLocation(font_shader_program_id, "ortho_projection"), 1, GL_FALSE, &ortho_projection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(text_VAO);

    int len = strlen(text);

    for (int i = 0; i < len; ++i){
        char c = text[i];
        Character ch = text_characters[c];
        float xpos = position.x + ch.bearing.x * scale;
        float ypos = position.y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        position.x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);

}
#endif // TEXTH
