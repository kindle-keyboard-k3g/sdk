package com.amazon.kindle.kindlet.ui;

import java.awt.Component;

/**
 * Base class for Kindle lightweight UI components in Personal Basis Profile 1.1.
 * Provides root abstraction for specialized E-Ink visual controls.
 */
public abstract class KComponent extends Component {

    /**
     * Constructs a new lightweight Kindle UI component.
     */
    public KComponent() {
        super();
    }
}
