package dev.damioir.sbacostumemap;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.app.AlertDialog;
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
                selectedIndex = -1;
                if (board != null) board.invalidate();
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

        Button edit = makeButton("EDIT");
        edit.setOnClickListener(v -> editSelected());
        footer.addView(edit, new LinearLayout.LayoutParams(dp(58), dp(48)));

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

    private void editSelected() {
        if (selectedIndex < 0 || selectedIndex >= items.size()) {
            status.setText("Select an item first");
            return;
        }
        MapItem item = items.get(selectedIndex);
        LinearLayout fields = new LinearLayout(this);
        fields.setOrientation(LinearLayout.VERTICAL);
        fields.setPadding(dp(20), dp(8), dp(20), dp(4));

        EditText sizeInput = new EditText(this);
        sizeInput.setSingleLine(true);
        sizeInput.setHint("Size (0.05 to 100)");
        sizeInput.setText(String.format(Locale.US, "%.2f", item.scaleX));
        fields.addView(sizeInput);

        EditText heightInput = new EditText(this);
        heightInput.setSingleLine(true);
        heightInput.setHint("Height");
        heightInput.setText(String.format(Locale.US, "%.2f", item.worldY));
        fields.addView(heightInput);

        EditText rotationInput = new EditText(this);
        rotationInput.setSingleLine(true);
        rotationInput.setHint("Rotation (degrees)");
        rotationInput.setText(String.format(Locale.US, "%.2f", item.yaw));
        fields.addView(rotationInput);

        EditText messageInput = null;
        EditText eventInput = null;
        EditText animationInput = null;
        if (item.kind.equals("message")) {
            messageInput = new EditText(this);
            messageInput.setHint("Message text");
            messageInput.setMinLines(2);
            messageInput.setText(item.text);
            fields.addView(messageInput);
        }
        if (item.kind.equals("trigger") || item.kind.equals("message") || item.kind.equals("animation")) {
            eventInput = new EditText(this);
            eventInput.setSingleLine(true);
            eventInput.setHint("Event key (e.g. event-1)");
            eventInput.setText(item.eventKey);
            fields.addView(eventInput);
        }
        if (item.kind.equals("animation")) {
            animationInput = new EditText(this);
            animationInput.setSingleLine(true);
            animationInput.setHint("Animation key");
            animationInput.setText(item.animationKey);
            fields.addView(animationInput);
        }

        final EditText messageField = messageInput;
        final EditText eventField = eventInput;
        final EditText animationField = animationInput;
        new AlertDialog.Builder(this)
                .setTitle("Edit " + item.kind.replace('_', ' '))
                .setView(fields)
                .setNegativeButton("CANCEL", null)
                .setPositiveButton("APPLY", (dialog, which) -> {
                    try {
                        float size = Float.parseFloat(sizeInput.getText().toString().trim());
                        float height = Float.parseFloat(heightInput.getText().toString().trim());
                        float rotation = Float.parseFloat(rotationInput.getText().toString().trim());
                        if (!Float.isFinite(size) || size < 0.05f || size > 100.0f) {
                            status.setText("Size must be between 0.05 and 100");
                            return;
                        }
                        if (!Float.isFinite(height) || Math.abs(height) > 5000.0f ||
                                !Float.isFinite(rotation) || Math.abs(rotation) > 360000.0f) {
                            status.setText("Height or rotation is out of range");
                            return;
                        }
                        if (eventField != null) {
                            String key = eventField.getText().toString().trim();
                            if ((item.kind.equals("trigger") || item.kind.equals("animation") || !key.isEmpty()) && !validKey(key)) {
                                status.setText("Event key: use letters, numbers, dot, dash, underscore");
                                return;
                            }
                            item.eventKey = key;
                        }
                        if (animationField != null) {
                            String key = animationField.getText().toString().trim();
                            if (!validKey(key)) {
                                status.setText("Animation key is required");
                                return;
                            }
                            item.animationKey = key;
                        }
                        if (messageField != null) {
                            String value = messageField.getText().toString();
                            if (value.trim().isEmpty() || value.length() > 1000) {
                                status.setText("Message must contain 1-1000 characters");
                                return;
                            }
                            item.text = value;
                        }
                        item.scaleX = size;
                        item.scaleY = size;
                        item.scaleZ = size;
                        item.worldY = height;
                        item.yaw = rotation;
                        board.invalidate();
                        status.setText("Updated " + item.kind.replace('_', ' '));
                    } catch (NumberFormatException error) {
                        status.setText("Enter a valid size");
                    }
                }).show();
    }

    private static boolean validKey(String value) {
        if (value.isEmpty() || value.length() > 64) return false;
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
        }
        return true;
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
                data.append("ELEMENT\t").append(item.id).append('\t').append(item.kind)
                        .append("\t\t")
                        .append(String.format(Locale.US, "%.2f", item.worldX())).append('\t')
                        .append(String.format(Locale.US, "%.2f", item.worldY)).append('\t')
                        .append(String.format(Locale.US, "%.2f", item.worldZ()))
                        .append('\t').append(String.format(Locale.US, "%.2f", item.yaw))
                        .append('\t').append(String.format(Locale.US, "%.2f", item.scaleX))
                        .append('\t').append(String.format(Locale.US, "%.2f", item.scaleY))
                        .append('\t').append(String.format(Locale.US, "%.2f", item.scaleZ))
                        .append('\t').append(escape(item.text)).append('\t')
                        .append(escape(item.eventKey)).append('\t')
                        .append(escape(item.animationKey)).append('\n');
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
                        MapItem item = new MapItem(id, kind, column, row);
                        item.worldY = Float.parseFloat(fields[5]);
                        item.yaw = Float.parseFloat(fields[7]);
                        item.scaleX = Float.parseFloat(fields[8]);
                        item.scaleY = Float.parseFloat(fields[9]);
                        item.scaleZ = Float.parseFloat(fields[10]);
                        item.text = unescape(fields[11]);
                        item.eventKey = unescape(fields[12]);
                        item.animationKey = unescape(fields[13]);
                        loaded.add(item);
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
        int column;
        int row;
        float worldY = 0.0f;
        float yaw = 0.0f;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
        String text = "";
        String eventKey = "";
        String animationKey = "";

        MapItem(String id, String kind, int column, int row) {
            this.id = id;
            this.kind = kind;
            this.column = column;
            this.row = row;
            if (kind.equals("message")) {
                text = "Hello bear!";
                eventKey = "event-1";
            } else if (kind.equals("trigger")) {
                eventKey = "event-1";
            } else if (kind.equals("animation")) {
                eventKey = "event-1";
                animationKey = "idle";
            }
        }

        float worldX() { return (column - 10) * 2.0f; }
        float worldZ() { return (row - 6) * 2.0f; }
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
            if (selectedIndex >= 0 && selectedIndex < items.size()) {
                MapItem selected = items.get(selectedIndex);
                selected.column = column;
                selected.row = row;
                invalidate();
                status.setText("Moved " + selected.kind.replace('_', ' '));
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
