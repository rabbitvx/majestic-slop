#pragma once
#include "font.h"
#include <string>

class c_font {
private:
	struct merged_font_source {
		std::string file_path;
		const ImWchar* ranges = nullptr;
	};

	unsigned char* data = nullptr;
	size_t data_size = 0;
	std::string file_path;
	std::unordered_map< float, ImFont* > fonts;
	ImWchar* ranges = nullptr;
	bool use_file_path = false;
	std::vector< merged_font_source > merged_fonts;
public:
	std::vector< float > should_init = { };

	void init( std::vector< float > sizes, bool add = true ) {
		for ( auto& sz : sizes ) {
			if ( add )
				should_init.emplace_back( sz );

			auto config = ImFontConfig( );
			config.FontDataOwnedByAtlas = false;
			config.PixelSnapH = true;

			ImFont* loaded_font = use_file_path
				? ImGui::GetIO( ).Fonts->AddFontFromFileTTF( file_path.c_str( ), sz * dpi_scale, &config, ranges )
				: ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF( data, data_size, sz * dpi_scale, &config, ranges );
			fonts.insert( { sz, loaded_font } );

			for ( const auto& merged_font : merged_fonts ) {
				auto merged_config = ImFontConfig( );
				merged_config.PixelSnapH = true;
				merged_config.MergeMode = true;
				merged_config.DstFont = loaded_font;
				ImGui::GetIO( ).Fonts->AddFontFromFileTTF(
					merged_font.file_path.c_str( ),
					sz * dpi_scale,
					&merged_config,
					const_cast< ImWchar* >( merged_font.ranges ) );
			}
		}
	}

	ImFont* get( float sz ) {
		auto it = fonts.find( sz );

		if ( it == fonts.end( ) ) {
			if ( std::find( should_init.begin( ), should_init.end( ), sz ) == should_init.end( ) )
				should_init.emplace_back( sz );

			return nullptr;
		}

		return it->second;
	}

	void set_data( const unsigned char* bytes, size_t size ) {
		delete[] data;

        data = new unsigned char[size];
        std::memcpy(data, bytes, size);

        data_size = size;
		use_file_path = false;
	}

	void set_file( const char* path ) {
		file_path = path;
		use_file_path = true;
	}

	void set_ranges( const ImWchar* _ranges ) {
		ranges = ( ImWchar* )_ranges;
	}

	void merge_file( const char* path, const ImWchar* merge_ranges ) {
		merged_fonts.push_back( merged_font_source { path, merge_ranges } );
	}

	auto& get_fonts( ) {
		return fonts;
	}
};
