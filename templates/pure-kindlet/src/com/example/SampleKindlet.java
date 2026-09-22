package com.example;

import com.amazon.kindle.kindlet.AbstractKindlet;
import com.amazon.kindle.kindlet.KindletContext;
import com.amazon.kindle.kindlet.KindletExecutionException;
import com.amazon.kindle.kindlet.ui.KLabel;
import java.awt.BorderLayout;
import java.awt.Container;

public class SampleKindlet extends AbstractKindlet {

    private KLabel label;

    public void create(KindletContext context) throws KindletExecutionException {
        super.create(context);
        Container root = context.getRootContainer();
        root.setLayout(new BorderLayout());

        label = new KLabel("Hello Kindle Keyboard!");
        root.add(label, BorderLayout.CENTER);
    }
}
