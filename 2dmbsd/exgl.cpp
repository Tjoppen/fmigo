
//-----------------------------------------------------------------------------

// Name: glEnable2D

// Desc: Enabled 2D primitive rendering by setting up the appropriate orthographic

//               perspectives and matrices.

//-----------------------------------------------------------------------------

void glEnable2D( void )

{

        GLint iViewport[4];



        // Get a copy of the viewport

        glGetIntegerv( GL_VIEWPORT, iViewport );



        // Save a copy of the projection matrix so that we can restore it 

        // when it's time to do 3D rendering again.

        glMatrixMode( GL_PROJECTION );

        glPushMatrix();

        glLoadIdentity();



        // Set up the orthographic projection

        glOrtho( iViewport[0], iViewport[0]+iViewport[2],

                         iViewport[1]+iViewport[3], iViewport[1], -1, 1 );

        glMatrixMode( GL_MODELVIEW );

        glPushMatrix();

        glLoadIdentity();



        // Make sure depth testing and lighting are disabled for 2D rendering until

        // we are finished rendering in 2D

        glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT );

        glDisable( GL_DEPTH_TEST );

        glDisable( GL_LIGHTING );

}





//-----------------------------------------------------------------------------

// Name: glDisable2D

// Desc: Disables 2D rendering and restores the previous matrix and render states

//               before they were modified.

//-----------------------------------------------------------------------------

void glDisable2D( void )

{

        glPopAttrib();

        glMatrixMode( GL_PROJECTION );

        glPopMatrix();

        glMatrixMode( GL_MODELVIEW );

        glPopMatrix();

}





//-----------------------------------------------------------------------------

// Name: InitScene

// Desc: Initializes extensions, textures, render states, etc. before rendering

//-----------------------------------------------------------------------------

int InitScene( void )

{

        // Is the extension supported on this driver/card?

        if( !glh_extension_supported( "GL_NV_texture_rectangle" ) )

        {

                printf( "ERROR: Texture rectangles not supported on this video card!" );

                Sleep(2000);

                exit(-1);

        }



        // NOTE: If your comp doesn't support GL_NV_texture_rectangle, you can try

        // using GL_EXT_texture_rectangle if you want, it should work fine.



        // Disable lighting

        glDisable( GL_LIGHTING );



        // Disable dithering

        glDisable( GL_DITHER );



        // Disable blending (for now)

        glDisable( GL_BLEND );



        // Disable depth testing

        glDisable( GL_DEPTH_TEST );



        return LoadSpriteTexture();

}





// Enable the texture rectangle extension

glEnable( GL_TEXTURE_RECTANGLE_NV );



// Generate one texture ID

glGenTextures( 1, &g_uTextureID );

// Bind the texture using GL_TEXTURE_RECTANGLE_NV

glBindTexture( GL_TEXTURE_RECTANGLE_NV, g_uTextureID );

// Enable bilinear filtering on this texture

glTexParameteri( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

glTexParameteri( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR );



// Write the 32-bit RGBA texture buffer to video memory

glTexImage2D( GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, pTexture_RGB->sizeX, pTexture_RGB->sizeY,

                          0, GL_RGBA, GL_UNSIGNED_BYTE, pTexture_RGBA );



// Save a copy of the texture's dimensions for later use

g_iTextureWidth = pTexture_RGB->sizeX;

g_iTextureHeight = pTexture_RGB->sizeY;



// Enable 2D rendering

glEnable2D();

        

// Make the sprite 2 times bigger (optional)

glScalef( 2.0f, 2.0f, 0.0f );



// Blend the color key into oblivion! (optional)

glEnable( GL_BLEND );

glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );



// Set the primitive color to white

glColor3f( 1.0f, 1.0f, 1.0f );

// Bind the texture to the polygons

glBindTexture( GL_TEXTURE_RECTANGLE_NV, g_uTextureID );



// Render a quad

// Instead of the using (s,t) coordinates, with the  GL_NV_texture_rectangle

// extension, you need to use the actual dimensions of the texture.

// This makes using 2D sprites for games and emulators much easier now

// that you won't have to convert :)

glBegin( GL_QUADS );

 glTexCoord2i( 0, g_iTextureHeight );                           

 glVertex2i( 0, 0 );

 glTexCoord2i( g_iTextureWidth, g_iTextureHeight );     

 glVertex2i( g_iTextureWidth, 0 );

 glTexCoord2i( g_iTextureWidth, 0 );    

 glVertex2i( g_iTextureWidth, g_iTextureHeight );

 glTexCoord2i( 0, 0 );          

 glVertex2i( 0, g_iTextureHeight );

glEnd();



// Disable 2D rendering

glDisable2D();

