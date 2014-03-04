#if !defined(HISTORYSTATS_GUARD_COLUMN_WORDCOUNT_H)
#define HISTORYSTATS_GUARD_COLUMN_WORDCOUNT_H

#include "colbase_words.h"

/*
 * ColWordCount
 */

class ColWordCount
	: public ColBaseWords
{
protected:
	int m_nVisMode;
	bool m_bDetail;

	OptionsCtrl::Radio m_hVisMode;
	OptionsCtrl::Check m_hDetail;

public:
	explicit ColWordCount();

protected:
	virtual const mu_text* impl_getUID() const { return con::ColWordCount; }
	virtual const mu_text* impl_getTitle() const { return i18n(muT("Word count")); }
	virtual const mu_text* impl_getDescription() const { return i18n(muT("Column holding the number of (distinct) words used by you, by your contact, or by both of you.")); }
	virtual void impl_copyConfig(const Column* pSource);
	virtual void impl_configRead(const SettingsTree& settings);
	virtual void impl_configWrite(SettingsTree& settings) const;
	virtual void impl_configToUI(OptionsCtrl& Opt, OptionsCtrl::Item hGroup);
	virtual void impl_configFromUI(OptionsCtrl& Opt);
	virtual void impl_contactDataFree(Contact& contact) const;
	virtual void impl_contactDataTransform(Contact& contact) const;
	virtual void impl_contactDataTransformCleanup(Contact& contact) const;
	virtual void impl_outputRenderHeader(ext::ostream& tos, int row, int rowSpan) const;
	virtual void impl_outputRenderRow(ext::ostream& tos, const Contact& contact, DisplayType display);
};

#endif // HISTORYSTATS_GUARD_COLUMN_WORDCOUNT_H