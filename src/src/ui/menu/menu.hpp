#pragma once

struct ID3D11Device;

namespace menu {
    void initialize( ID3D11Device* device );
    void shutdown( );
    void render( bool menu_open );
}
