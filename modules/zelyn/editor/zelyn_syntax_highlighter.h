/**************************************************************************/
/*  zelyn_syntax_highlighter.h                                            */
/**************************************************************************/

#pragma once

#include "editor/script/syntax_highlighters.h"

class ZelynSyntaxHighlighter : public EditorSyntaxHighlighter {
	GDCLASS(ZelynSyntaxHighlighter, EditorSyntaxHighlighter);

private:
	Ref<CodeHighlighter> highlighter;

protected:
	static void _bind_methods();

public:
	virtual String _get_name() const override { return "Zelyn"; }
	virtual PackedStringArray _get_supported_languages() const override;
	virtual Ref<EditorSyntaxHighlighter> _create() const override;

	virtual void _update_cache() override;
	virtual Dictionary _get_line_syntax_highlighting_impl(int p_line) override;

	ZelynSyntaxHighlighter();
	~ZelynSyntaxHighlighter();
};
