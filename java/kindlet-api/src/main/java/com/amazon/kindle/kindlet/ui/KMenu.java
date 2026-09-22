package com.amazon.kindle.kindlet.ui;

import java.util.Vector;

/**
 * Menu abstraction for Kindle applications.
 */
public class KMenu {

    private final Vector items = new Vector();

    public void add(KMenuItem item) {
        if (item != null) {
            items.addElement(item);
        }
    }

    public void remove(KMenuItem item) {
        items.removeElement(item);
    }

    public int getItemCount() {
        return items.size();
    }

    public KMenuItem getItem(int index) {
        return (KMenuItem) items.elementAt(index);
    }
}
