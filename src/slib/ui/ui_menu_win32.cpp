/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/menu.h"

#include "slib/ui/platform.h"
#include "slib/ui/window.h"
#include "slib/core/safe_static.h"

#include "ui_core_win32.h"

namespace slib
{

	class _priv_Win32_Menu;
	typedef CHashMap<HMENU, WeakRef<_priv_Win32_Menu> > _priv_UiMenuMap;
	SLIB_SAFE_STATIC_GETTER(_priv_UiMenuMap, _priv_UI_getMenu)

	class _priv_Win32_MenuItem : public MenuItem
	{
		SLIB_DECLARE_OBJECT

	public:
		HBITMAP m_hbmChecked;
		HBITMAP m_hbmUnchecked;

	public:
		_priv_Win32_MenuItem()
		{
			m_hbmChecked = NULL;
			m_hbmUnchecked = NULL;
		}

		~_priv_Win32_MenuItem()
		{
			if (m_hbmChecked) {
				::DeleteObject(m_hbmChecked);
			}
			if (m_hbmUnchecked) {
				::DeleteObject(m_hbmUnchecked);
			}
		}

		static Ref<_priv_Win32_MenuItem> create(_priv_Win32_Menu* parent, sl_uint32 index, const MenuItemParam& param);

		static String makeText(const String& title, const KeycodeAndModifiers& shortcutKey, const KeycodeAndModifiers& secondShortcutKey);

		void _updateText();

		void setText(const String& text) override
		{
			MenuItem::setText(text);
			_updateText();
		}

		void setShortcutKey(const KeycodeAndModifiers& km) override
		{
			MenuItem::setShortcutKey(km);
			_updateText();
		}

		void setSecondShortcutKey(const KeycodeAndModifiers& km) override
		{
			MenuItem::setSecondShortcutKey(km);
			_updateText();
		}

		void _updateState();

		void setEnabled(sl_bool flag) override
		{
			MenuItem::setEnabled(flag);
			_updateState();
		}

		void setChecked(sl_bool flag) override
		{
			MenuItem::setChecked(flag);
			_updateState();
		}

		void setIcon(const Ref<Bitmap>& icon) override;

		void setCheckedIcon(const Ref<Bitmap>& icon) override;

		void setSubmenu(const Ref<Menu>& menu) override;

	};

	SLIB_DEFINE_OBJECT(_priv_Win32_MenuItem, MenuItem)

	class _priv_Win32_Menu : public Menu
	{
		SLIB_DECLARE_OBJECT

	public:
		HMENU m_hMenu;

	public:
		_priv_Win32_Menu()
		{
			m_hMenu = NULL;
		}

		~_priv_Win32_Menu()
		{
			if (m_hMenu) {
				::DestroyMenu(m_hMenu);
				_priv_UiMenuMap* map = _priv_UI_getMenu();
				if (map) {
					map->remove(m_hMenu);
				}
			}
		}

		static Ref<_priv_Win32_Menu> create()
		{
			HMENU hMenu = ::CreateMenu();
			if (hMenu) {
				MENUINFO mi;
				mi.cbSize = sizeof(mi);
				mi.fMask = MIM_STYLE;
				mi.dwStyle = MNS_NOTIFYBYPOS;
				if (::SetMenuInfo(hMenu, &mi)) {
					Ref<_priv_Win32_Menu> ret = new _priv_Win32_Menu();
					if (ret.isNotNull()) {
						ret->m_hMenu = hMenu;
						_priv_UiMenuMap* map = _priv_UI_getMenu();
						if (map) {
							map->put(hMenu, ret);
						}
						return ret;
					}
				}
				::DestroyMenu(hMenu);
			}
			return sl_null;
		}

		Ref<MenuItem> addMenuItem(const MenuItemParam& param) override
		{
			return insertMenuItem(SLIB_UINT32_MAX, param);
		}

		Ref<MenuItem> insertMenuItem(sl_uint32 index, const MenuItemParam& param) override
		{
			ObjectLocker lock(this);
			sl_uint32 n = (sl_uint32)(m_items.getCount());
			if (index > n) {
				index = n;
			}
			Ref<MenuItem> item = _priv_Win32_MenuItem::create(this, index, param);
			if (item.isNotNull()) {
				m_items.insert(index, item);
				return item;
			}
			return sl_null;
		}

		Ref<MenuItem> addSeparator() override
		{
			return insertSeparator(SLIB_UINT32_MAX);
		}
		
		Ref<MenuItem> insertSeparator(sl_uint32 index) override
		{
			ObjectLocker lock(this);
			sl_uint32 n = (sl_uint32)(m_items.getCount());
			if (index > n) {
				index = n;
			}
			::MENUITEMINFOW mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_FTYPE;
			mii.fType = MFT_SEPARATOR;
			if (::InsertMenuItemW(m_hMenu, index, TRUE, &mii)) {
				Ref<MenuItem> item = MenuItem::createSeparator();
				if (item.isNotNull()) {
					m_items.insert(index, item);
					return item;
				}
			}
			return sl_null;
		}

		void removeMenuItem(sl_uint32 index) override
		{
			ObjectLocker lock(this);
			if (index < m_items.getCount()) {
				::RemoveMenu(m_hMenu, index, MF_BYPOSITION);
				m_items.removeAt(index);
			}
		}

		void removeMenuItem(const Ref<MenuItem>& item) override
		{
			ObjectLocker lock(this);
			sl_reg index = m_items.indexOf(item);
			if (index >= 0) {
				::RemoveMenu(m_hMenu, (sl_uint32)index, MF_BYPOSITION);
				m_items.removeAt(index);
			}
		}

		void show(sl_ui_pos x, sl_ui_pos y) override
		{
			Win32_UI_Shared* shared = Win32_UI_Shared::get();
			if (shared) {
				::TrackPopupMenuEx(m_hMenu, 0, (int)x, (int)y, shared->hWndMessage, NULL);
			}
		}

		friend class _priv_Win32_MenuItem;
	};

	SLIB_DEFINE_OBJECT(_priv_Win32_Menu, Menu)

	Ref<Menu> Menu::create()
	{
		return _priv_Win32_Menu::create();
	}

	Ref<_priv_Win32_MenuItem> _priv_Win32_MenuItem::create(_priv_Win32_Menu* parent, sl_uint32 index, const MenuItemParam& param)
	{
		MENUITEMINFOW mii;
		Base::zeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(MENUITEMINFOW);
		mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_STRING;
		mii.fType = MFT_STRING;
		mii.fState = 0;
		if (param.flagChecked) {
			mii.fState |= MFS_CHECKED;
		}
		if (!(param.flagEnabled)) {
			mii.fState |= MFS_DISABLED;
		}
		if (param.submenu.isNotNull()) {
			mii.hSubMenu = UIPlatform::getMenuHandle(param.submenu);
			if (mii.hSubMenu) {
				mii.fMask |= MIIM_SUBMENU;
			}
		}
		mii.hbmpUnchecked = GraphicsPlatform::createDIBFromBitmap(param.icon);
		mii.hbmpChecked = GraphicsPlatform::createDIBFromBitmap(param.checkedIcon);
		if (mii.hbmpUnchecked || mii.hbmpChecked) {
			mii.fMask |= MIIM_CHECKMARKS;
		}
		String16 text = makeText(param.text, param.shortcutKey, param.secondShortcutKey);
		mii.dwTypeData = (LPWSTR)(text.getData());
		if (::InsertMenuItemW(parent->m_hMenu, index, TRUE, &mii)) {
			Ref<_priv_Win32_MenuItem> ret = new _priv_Win32_MenuItem;
			if (ret.isNotNull()) {
				ret->m_parent = parent;
				ret->m_text = param.text;
				ret->m_shortcutKey = param.shortcutKey;
				ret->m_secondShortcutKey = param.secondShortcutKey;
				ret->m_flagEnabled = param.flagEnabled;
				ret->m_flagChecked = param.flagChecked;
				ret->m_icon = param.icon;
				ret->m_checkedIcon = param.checkedIcon;
				ret->m_submenu = param.submenu;
				ret->m_action = param.action;
				ret->m_hbmUnchecked = mii.hbmpUnchecked;
				ret->m_hbmChecked = mii.hbmpChecked;
				return ret;
			}
		}
		if (mii.hbmpChecked) {
			::DeleteObject(mii.hbmpChecked);
		}
		if (mii.hbmpUnchecked) {
			::DeleteObject(mii.hbmpUnchecked);
		}
		return sl_null;
	}

	String _priv_Win32_MenuItem::makeText(const String& title, const KeycodeAndModifiers& shortcutKey, const KeycodeAndModifiers& secondShortcutKey)
	{
		String text = title;
		if (shortcutKey.getKeycode() != Keycode::Unknown) {
			text += "\t";
			text += shortcutKey.toString();
			if (secondShortcutKey.getKeycode() != Keycode::Unknown) {
				text += ", ";
				text += secondShortcutKey.toString();
			}
		} else {
			if (secondShortcutKey.getKeycode() != Keycode::Unknown) {
				text += "\t";
				text += secondShortcutKey.toString();
			}
		}
		return text;
	}

#define MENU_ITEM_SET_PROLOG \
	Ref<_priv_Win32_Menu> parent(WeakRef<_priv_Win32_Menu>::from(m_parent)); \
	if (parent.isNull()) { \
		return; \
	} \
	ObjectLocker lock(parent.get()); \
	sl_reg _index = parent->m_items.indexOf(this); \
	if (_index < 0) { \
		return; \
	} \
	HMENU hMenu = parent->m_hMenu; \
	sl_uint32 index = (sl_uint32)_index; \
	MENUITEMINFOW mii; \
	Base::zeroMemory(&mii, sizeof(mii)); \
	mii.cbSize = sizeof(mii);

	void _priv_Win32_MenuItem::_updateText()
	{
		MENU_ITEM_SET_PROLOG;
		mii.fMask = MIIM_STRING;
		String16 text = makeText(m_text, m_shortcutKey, m_secondShortcutKey);
		mii.dwTypeData = (LPWSTR)(text.getData());
		::SetMenuItemInfoW(hMenu, index, TRUE, &mii);
	}

	void _priv_Win32_MenuItem::_updateState()
	{
		MENU_ITEM_SET_PROLOG;
		mii.fMask = MIIM_STATE;
		mii.fState = 0;
		if (m_flagChecked) {
			mii.fState |= MFS_CHECKED;
		}
		if (!(m_flagEnabled)) {
			mii.fState |= MFS_DISABLED;
		}
		::SetMenuItemInfoW(hMenu, index, TRUE, &mii);
	}

	void _priv_Win32_MenuItem::setIcon(const Ref<Bitmap>& icon)
	{
		MenuItem::setIcon(icon);
		MENU_ITEM_SET_PROLOG;
		if (m_hbmUnchecked) {
			::DeleteObject(m_hbmUnchecked);
		}
		m_hbmUnchecked = GraphicsPlatform::createDIBFromBitmap(icon);
		mii.fMask = MIIM_CHECKMARKS;
		mii.hbmpUnchecked = m_hbmUnchecked;
		mii.hbmpChecked = m_hbmChecked;
		::SetMenuItemInfoW(hMenu, index, TRUE, &mii);
	}

	void _priv_Win32_MenuItem::setCheckedIcon(const Ref<Bitmap>& icon)
	{
		MenuItem::setIcon(icon);
		MENU_ITEM_SET_PROLOG;
		if (m_hbmChecked) {
			::DeleteObject(m_hbmChecked);
		}
		m_hbmChecked = GraphicsPlatform::createDIBFromBitmap(icon);
		mii.fMask = MIIM_CHECKMARKS;
		mii.hbmpUnchecked = m_hbmUnchecked;
		mii.hbmpChecked = m_hbmChecked;
		::SetMenuItemInfoW(hMenu, index, TRUE, &mii);
	}

	void _priv_Win32_MenuItem::setSubmenu(const Ref<Menu>& menu)
	{
		MenuItem::setSubmenu(menu);
		MENU_ITEM_SET_PROLOG;
		mii.fMask = MIIM_SUBMENU;
		mii.hSubMenu = UIPlatform::getMenuHandle(menu);
		::SetMenuItemInfoW(hMenu, index, TRUE, &mii);
	}


	HMENU UIPlatform::getMenuHandle(const Ref<Menu>& menu)
	{
		if (_priv_Win32_Menu* _menu = CastInstance<_priv_Win32_Menu>(menu.get())) {
			return _menu->m_hMenu;
		}
		return NULL;
	}

	Ref<Menu> UIPlatform::getMenu(HMENU hMenu)
	{
		_priv_UiMenuMap* map = _priv_UI_getMenu();
		if (map) {
			return map->getValue(hMenu, WeakRef<_priv_Win32_Menu>::null());
		}
		return sl_null;
	}

	void _priv_Win32_processMenuCommand(WPARAM wParam, LPARAM lParam)
	{
		HMENU hMenu = (HMENU)(lParam);
		sl_uint32 index = (sl_uint32)(wParam);
		_priv_UiMenuMap* map = _priv_UI_getMenu();
		if (map) {
			Ref<_priv_Win32_Menu> menu(map->getValue(hMenu, WeakRef<_priv_Win32_Menu>::null()));
			if (menu.isNotNull()) {
				Ref<MenuItem> item = menu->getMenuItem(index);
				if (item.isNotNull()) {
					(item->getAction())();
				}
			}
		}
	}

	sl_bool _priv_Win32_processMenuShortcutKey(MSG& msg)
	{
		if (msg.message != WM_KEYDOWN) {
			return sl_false;
		}
		HWND hWnd = Windows::getRootWindow(msg.hwnd);
		if (hWnd) {
			Ref<WindowInstance> instance = UIPlatform::getWindowInstance(hWnd);
			if (instance.isNotNull()) {
				Ref<Window> window = instance->getWindow();
				if (window.isNotNull()) {
					Ref<Menu> menu = window->getMenu();
					if (menu.isNotNull()) {
						Keycode keycode = UIEvent::getKeycodeFromSystemKeycode((sl_uint32)(msg.wParam));
						KeycodeAndModifiers km;
						km.setKeycode(keycode);
						if (::GetKeyState(VK_CONTROL) & 0x8000) {
							km.setControlKey();
						}
						if (::GetKeyState(VK_SHIFT) & 0x8000) {
							km.setShiftKey();
						}
						if (::GetKeyState(VK_MENU) & 0x8000) {
							km.setAltKey();
						}
						if ((::GetKeyState(VK_LWIN) & 0x8000) || (::GetKeyState(VK_RWIN) & 0x8000)) {
							km.setWindowsKey();
						}
						if (km != 0) {
							return menu->processShortcutKey(km);
						}
					}
				}
			}
		}
		return sl_false;
	}

}

#endif
