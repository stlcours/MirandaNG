

class CSelectCryptoDialog : public CDlgBase
{
	CCtrlCombo m_combo;
	CRYPTO_PROVIDER **m_provs;
	size_t m_provscount;
	CRYPTO_PROVIDER *m_selected;
public:
	CSelectCryptoDialog(CRYPTO_PROVIDER **provs, size_t count) : 
		CDlgBase(g_hInst, IDD_SELECT_CRYPTOPROVIDER), 
		m_combo(this, IDC_SELECTCRYPT_COMBO), 
		m_provs(provs), 
		m_provscount(count),
		m_selected(nullptr)
	{
	}
	void OnInitDialog()
	{
		for (size_t i = 0; i < m_provscount; i++)
		{
			CRYPTO_PROVIDER *prov = m_provs[i];
			m_combo.AddStringA(prov->pszName, i);
		}
		m_combo.SetCurSel(0);
	}
	void OnClose()
	{
		m_selected = m_provs[ m_combo.GetItemData(m_combo.GetCurSel()) ];
	}

	inline CRYPTO_PROVIDER* GetSelected()
	{
		return m_selected;
	}

};