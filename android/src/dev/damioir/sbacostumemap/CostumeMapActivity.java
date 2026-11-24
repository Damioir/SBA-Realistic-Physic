package dev.damioir.sbacostumemap;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.unity3d.player.UnityPlayerActivity;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Locale;

/** SBAModLoader replaces the game's launch activity with this subclass. */
public final class CostumeMapActivity extends UnityPlayerActivity {
    private static final int BG = Color.rgb(17, 22, 34);
    private static final int PANEL = Color.rgb(29, 37, 54);
    private static final int ACCENT = Color.rgb(0, 220, 190);

    private FrameLayout root;
    private FrameLayout editorLayer;
    private MapBoard board;
    private EditText titleInput;
    private TextView status;
    private final ArrayList<MapItem> items = new ArrayList<>();
    private String selectedTool = "prop";
    private int selectedIndex = -1;
    private int nextId = 1;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        root = (FrameLayout) getWindow().getDecorView();
        Button launch = new Button(this);
        launch.setText("MAP EDITOR");
        launch.setTextColor(Color.WHITE);
        launch.setBackgroundColor(0xDD142333);
        launch.setOnClickListener(v -> openEditor());
        FrameLayout.LayoutParams buttonParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dp(52), Gravity.BOTTOM | Gravity.END);
        buttonParams.setMargins(0, 0, dp(16), dp(18));
        root.addView(launch, buttonParams);
    }

    private void openEditor() {
        if (editorLayer != null) return;
        editorLayer = new FrameLayout(this);
        editorLayer.setBackgroundColor(0xF2111622);

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(dp(12), dp(10), dp(12), dp(10));
        editorLayer.addView(column, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout header = new LinearLayout(this);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setBackgroundColor(PANEL);
        titleInput = new EditText(this);
        titleInput.setSingleLine(true);
        titleInput.setTextColor(Color.WHITE);
        titleInput.setHintTextColor(0xFF91A0B7);
        titleInput.setHint("Map name");
        titleInput.setText("My Costume Map");
        header.addView(titleInput, new LinearLayout.LayoutParams(0, dp(48), 1));

        Button close = makeButton("CLOSE");
        close.setOnClickListener(v -> closeEditor());
        header.addView(close, new LinearLayout.LayoutParams(dp(84), dp(48)));
        column.addView(header);

        board = new MapBoard();
        LinearLayout.LayoutParams boardParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1);
        boardParams.topMargin = dp(10);
        boardParams.bottomMargin = dp(10);
        column.addView(board, boardParams);

        HorizontalScrollView paletteScroll = new HorizontalScrollView(this);
        paletteScroll.setHorizontalScrollBarEnabled(false);
        LinearLayout palette = new LinearLayout(this);
        palette.setBackgroundColor(PANEL);
        palette.setPadding(dp(4), dp(4), dp(4), dp(4));
        for (String tool : new String[]{"prop", "chair", "death_block", "message", "trigger", "animation"}) {
            Button choice = makeButton(tool.toUpperCase(Locale.ROOT).replace('_', ' '));
            choice.setOnClickListener(v -> {
                selectedTool = tool;
                status.setText("Selected: " + tool.replace('_', ' '));
            });
            palette.addView(choice, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT, dp(46)));
        }
        paletteScroll.addView(palette);
        column.addView(paletteScroll);

        LinearLayout footer = new LinearLayout(this);
        footer.setGravity(Gravity.CENTER_VERTICAL);
        status = new TextView(this);
        status.setTextColor(ACCENT);
        status.setText("Tap the grid to place an item");
        footer.addView(status, new LinearLayout.LayoutParams(0, dp(48), 1));

        Button undo = makeButton("UNDO");
        undo.setOnClickListener(v -> {
            if (!items.isEmpty()) {
                items.remove(items.size() - 1);
                selectedIndex = -1;
                board.invalidate();
                status.setText("Last item removed");
            }
        });
        footer.addView(undo, new LinearLayout.LayoutParams(dp(64), dp(48)));

        Button remove = makeButton("DELETE");
        remove.setOnClickListener(v -> {
            if (selectedIndex >= 0 && selectedIndex < items.size()) {
                items.remove(selectedIndex);
                selectedIndex = -1;
                board.invalidate();
                status.setText("Item deleted");
            }
        });
        footer.addView(remove, new LinearLayout.LayoutParams(dp(68), dp(48)));

        Button load = makeButton("LOAD");
        load.setOnClickListener(v -> loadMap());
        footer.addView(load, new LinearLayout.LayoutParams(dp(64), dp(48)));

        Button save = makeButton("SAVE");
        save.setOnClickListener(v -> saveMap());
        footer.addView(save, new LinearLayout.LayoutParams(dp(64), dp(48)));
        column.addView(footer);

        root.addView(editorLayer, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    private void closeEditor() {
        if (editorLayer != null) root.removeView(editorLayer);
        editorLayer = null;
        board = null;
    }

    private Button makeButton(String label) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextSize(11);
        button.setTextColor(Color.WHITE);
        button.setBackgroundColor(0xFF26344A);
        return button;
    }

    private void saveMap() {
        try {
            File directory = getExternalFilesDir("CostumeMaps");
            if (directory == null || (!directory.exists() && !directory.mkdirs())) {
                status.setText("Could not create map folder");
                return;
            }
            String title = titleInput.getText().toString().trim();
            if (title.isEmpty()) title = "My Costume Map";
            StringBuilder data = new StringBuilder("SBA_MAP\t1\n");
            data.append("META\tcostume-map\t").append(escape(title)).append("\t\n");
            for (MapItem item : items) {
                String text = item.kind.equals("message") ? "Hello bear!" : "";
                String event = item.kind.equals("trigger") ? "event-1" : "";
                String animation = item.kind.equals("animation") ? "idle" : "";
                data.append("ELEMENT\t").append(item.id).append('\t').append(item.kind)
                        .append("\t\t")
                        .append(String.format(Locale.US, "%.2f", item.worldX)).append("\t0.00\t")
                        .append(String.format(Locale.US, "%.2f", item.worldZ))
                        .append("\t0.00\t1.00\t1.00\t1.00\t")
                        .append(escape(text)).append('\t').append(escape(event)).append('\t')
                        .append(escape(animation)).append('\n');
            }
            File target = new File(directory, "map1.sbamap");
            File temporary = new File(directory, "map1.sbamap.tmp");
            try (FileOutputStream output = new FileOutputStream(temporary)) {
                output.write(data.toString().getBytes(StandardCharsets.UTF_8));
                output.getFD().sync();
            }
            if (!temporary.renameTo(target)) {
                temporary.delete();
                status.setText("Could not save map");
                return;
            }
            status.setText("Saved " + items.size() + "; restart game to load");
        } catch (Exception e) {
            status.setText("Save failed: " + e.getClass().getSimpleName());
        }
    }

    private void loadMap() {
        try {
            File directory = getExternalFilesDir("CostumeMaps");
            File target = directory == null ? null : new File(directory, "map1.sbamap");
            if (target == null || !target.isFile()) {
                status.setText("No saved map yet");
                return;
            }
            ArrayList<MapItem> loaded = new ArrayList<>();
            String loadedTitle = "My Costume Map";
            int largestId = 0;
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                    new FileInputStream(target), StandardCharsets.UTF_8))) {
                String line;
                boolean header = false;
                while ((line = reader.readLine()) != null) {
                    if (!header) {
                        if (!line.equals("SBA_MAP\t1")) throw new IllegalArgumentException("Bad map header");
                        header = true;
                        continue;
                    }
                    String[] fields = line.split("\t", -1);
                    if (fields.length == 4 && fields[0].equals("META")) {
                        loadedTitle = unescape(fields[2]);
                    } else if (fields.length == 14 && fields[0].equals("ELEMENT")) {
                        String kind = fields[2];
                        if (!kind.equals("prop") && !kind.equals("chair") &&
                                !kind.equals("death_block") && !kind.equals("message") &&
                                !kind.equals("trigger") && !kind.equals("animation")) continue;
                        int column = Math.round(Float.parseFloat(fields[4]) / 2.0f + 10.0f);
                        int row = Math.round(Float.parseFloat(fields[6]) / 2.0f + 6.0f);
                        column = Math.max(0, Math.min(19, column));
                        row = Math.max(0, Math.min(11, row));
                        String id = unescape(fields[1]);
                        loaded.add(new MapItem(id, kind, column, row));
                        if (id.startsWith("item-")) {
                            try { largestId = Math.max(largestId, Integer.parseInt(id.substring(5))); }
                            catch (NumberFormatException ignored) { }
                        }
                    }
                }
                if (!header) throw new IllegalArgumentException("Empty map");
            }
            items.clear();
            items.addAll(loaded);
            selectedIndex = -1;
            nextId = largestId + 1;
            titleInput.setText(loadedTitle);
            board.invalidate();
            status.setText("Loaded " + items.size() + " items");
        } catch (Exception e) {
            status.setText("Load failed: " + e.getClass().getSimpleName());
        }
    }

    private static String unescape(String value) {
        StringBuilder output = new StringBuilder();
        for (int i = 0; i < value.length(); i++) {
            char current = value.charAt(i);
            if (current == '%' && i + 2 < value.length()) {
                try {
                    output.append((char) Integer.parseInt(value.substring(i + 1, i + 3), 16));
                    i += 2;
                    continue;
                } catch (NumberFormatException ignored) { }
            }
            output.append(current);
        }
        return output.toString();
    }

    private static String escape(String value) {
        return value.replace("%", "%25").replace("\t", "%09")
                .replace("\n", "%0A").replace("\r", "%0D");
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }

    private static final class MapItem {
        final String id;
        final String kind;
        final int column;
        final int row;
        final float worldX;
        final float worldZ;

        MapItem(String id, String kind, int column, int row) {
            this.id = id;
            this.kind = kind;
            this.column = column;
            this.row = row;
            this.worldX = (column - 10) * 2.0f;
            this.worldZ = (row - 6) * 2.0f;
        }
    }

    private final class MapBoard extends View {
        private static final int COLUMNS = 20;
        private static final int ROWS = 12;
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

        MapBoard() {
            super(CostumeMapActivity.this);
            setBackgroundColor(Color.rgb(12, 17, 27));
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float cellWidth = getWidth() / (float) COLUMNS;
            float cellHeight = getHeight() / (float) ROWS;
            paint.setColor(0xFF263044);
            paint.setStrokeWidth(dp(1));
            for (int x = 0; x <= COLUMNS; x++) {
                float px = x * cellWidth;
                canvas.drawLine(px, 0, px, getHeight(), paint);
            }
            for (int y = 0; y <= ROWS; y++) {
                float py = y * cellHeight;
                canvas.drawLine(0, py, getWidth(), py, paint);
            }
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTextSize(Math.min(cellWidth, cellHeight) * 0.42f);
            for (int i = 0; i < items.size(); i++) {
                MapItem item = items.get(i);
                float left = item.column * cellWidth;
                float top = item.row * cellHeight;
                int color = colorFor(item.kind);
                paint.setColor(color);
                canvas.drawRoundRect(left + 3, top + 3, left + cellWidth - 3,
                        top + cellHeight - 3, dp(4), dp(4), paint);
                if (i == selectedIndex) {
                    paint.setStyle(Paint.Style.STROKE);
                    paint.setStrokeWidth(dp(3));
                    paint.setColor(Color.WHITE);
                    canvas.drawRect(left + 2, top + 2, left + cellWidth - 2,
                            top + cellHeight - 2, paint);
                    paint.setStyle(Paint.Style.FILL);
                }
                paint.setColor(Color.WHITE);
                canvas.drawText(shortName(item.kind), left + cellWidth / 2,
                        top + cellHeight / 2 + paint.getTextSize() / 3, paint);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() != MotionEvent.ACTION_UP) return true;
            int column = Math.max(0, Math.min(COLUMNS - 1,
                    (int) (event.getX() / Math.max(1, getWidth()) * COLUMNS)));
            int row = Math.max(0, Math.min(ROWS - 1,
                    (int) (event.getY() / Math.max(1, getHeight()) * ROWS)));
            for (int i = items.size() - 1; i >= 0; i--) {
                MapItem item = items.get(i);
                if (item.column == column && item.row == row) {
                    selectedIndex = i;
                    invalidate();
                    status.setText("Selected " + item.kind.replace('_', ' '));
                    return true;
                }
            }
            if (items.size() >= 2000) {
                status.setText("Map limit reached");
                return true;
            }
            items.add(new MapItem("item-" + nextId++, selectedTool, column, row));
            selectedIndex = items.size() - 1;
            invalidate();
            status.setText("Placed " + selectedTool.replace('_', ' '));
            return true;
        }

        private int colorFor(String kind) {
            if (kind.equals("chair")) return Color.rgb(0, 170, 210);
            if (kind.equals("death_block")) return Color.rgb(235, 65, 80);
            if (kind.equals("message")) return Color.rgb(130, 90, 220);
            if (kind.equals("trigger")) return Color.rgb(40, 180, 100);
            if (kind.equals("animation")) return Color.rgb(220, 150, 40);
            return ACCENT;
        }

        private String shortName(String kind) {
            if (kind.equals("death_block")) return "DEAD";
            if (kind.equals("animation")) return "ANIM";
            return kind.toUpperCase(Locale.ROOT);
        }
    }
}
