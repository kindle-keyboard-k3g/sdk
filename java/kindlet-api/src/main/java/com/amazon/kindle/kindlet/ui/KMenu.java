package com.amazon.kindle.kindlet.ui;

import java.util.Vector;

/**
 * Menu abstraction representing top-level action menus in Kindle applications.
 */
public class KMenu {

    private final Vector items = new Vector();

    /**
     * Adds an item to the menu.
     *
     * @param item the KMenuItem to append
     */
    public void add(KMenuItem item) {
        if (item != null) {
            items.addElement(item);
        }
    }

    /**
     * Removes an item from the menu.
     *
     * @param item the KMenuItem to remove
     */
    public void remove(KMenuItem item) {
        items.removeElement(item);
    }

    /**
     * Returns the number of items in the menu.
     *
     * @return the item count
     */
    public int getItemCount() {
        return items.size();
    }

    /**
     * Returns the menu item at the specified index.
     *
     * @param index zero-based index
     * @return the KMenuItem at that position
     */
    public KMenuItem getItem(int index) {
        return (KMenuItem) items.elementAt(index);
    }
}
