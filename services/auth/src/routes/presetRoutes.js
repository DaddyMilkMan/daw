const express = require('express');
const router = express.Router();
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { protect } = require('../middleware/authMiddleware');
const Preset = require('../models/Preset');
const Project = require('../models/Project');

// Configure storage for preset files
const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        const uploadDir = 'uploads/presets/';
        if (!fs.existsSync(uploadDir)) {
            fs.mkdirSync(uploadDir, { recursive: true });
        }
        cb(null, uploadDir);
    },
    filename: function (req, file, cb) {
        const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1E9);
        cb(null, 'preset-' + req.user._id + '-' + uniqueSuffix + path.extname(file.originalname));
    }
});

const upload = multer({
    storage: storage,
    limits: { fileSize: 10 * 1024 * 1024 }, // 10MB limit for presets
    fileFilter: (req, file, cb) => {
        // Allow .zpreset, .fxp, .fxb and similar preset formats
        const allowedExtensions = ['.zpreset', '.fxp', '.fxb', '.vstpreset', '.aupreset'];
        const ext = path.extname(file.originalname).toLowerCase();
        if (allowedExtensions.includes(ext)) {
            cb(null, true);
        } else {
            cb(new Error('Invalid preset file format'), false);
        }
    }
});

// =============================================================================
// PUBLIC ROUTES (still require auth but show public presets)
// =============================================================================

// @desc    Get global feed of public presets
// @route   GET /api/presets/public
// @access  Public (no auth required for browsing)
router.get('/public', async (req, res) => {
    try {
        const page = parseInt(req.query.page) || 1;
        const limit = parseInt(req.query.limit) || 50;
        const skip = (page - 1) * limit;

        // Optional filters
        const query = { isPublic: true };

        // Filter by plugin
        if (req.query.pluginId) {
            query.pluginId = req.query.pluginId;
        }

        // Filter by category
        if (req.query.category) {
            query.category = req.query.category;
        }

        // Filter by tags
        if (req.query.tags) {
            const tags = req.query.tags.split(',');
            query.tags = { $in: tags };
        }

        // Sort options
        let sortOptions = { createdAt: -1 }; // Default: newest first
        if (req.query.sort === 'popular') {
            sortOptions = { downloads: -1, createdAt: -1 };
        } else if (req.query.sort === 'likes') {
            sortOptions = { likes: -1, createdAt: -1 };
        }

        const presets = await Preset.find(query)
            .populate('owner', 'name avatar')
            .sort(sortOptions)
            .skip(skip)
            .limit(limit)
            .lean();

        const total = await Preset.countDocuments(query);

        res.json({
            presets,
            pagination: {
                page,
                limit,
                total,
                pages: Math.ceil(total / limit)
            }
        });
    } catch (error) {
        console.error('Error fetching public presets:', error);
        res.status(500).json({ message: 'Server error fetching presets' });
    }
});

// @desc    Search public presets
// @route   GET /api/presets/search
// @access  Public
router.get('/search', async (req, res) => {
    try {
        const { q, pluginId, category } = req.query;

        if (!q || q.length < 2) {
            return res.status(400).json({ message: 'Search query must be at least 2 characters' });
        }

        const query = {
            isPublic: true,
            $or: [
                { name: { $regex: q, $options: 'i' } },
                { description: { $regex: q, $options: 'i' } },
                { tags: { $regex: q, $options: 'i' } }
            ]
        };

        if (pluginId) query.pluginId = pluginId;
        if (category) query.category = category;

        const presets = await Preset.find(query)
            .populate('owner', 'name avatar')
            .sort({ downloads: -1 })
            .limit(50)
            .lean();

        res.json({ presets });
    } catch (error) {
        console.error('Error searching presets:', error);
        res.status(500).json({ message: 'Server error searching presets' });
    }
});

// @desc    Download a preset file
// @route   GET /api/presets/:id/download
// @access  Public (for public presets) / Private (for private presets)
router.get('/:id/download', async (req, res) => {
    try {
        const preset = await Preset.findById(req.params.id);

        if (!preset) {
            return res.status(404).json({ message: 'Preset not found' });
        }

        // Check access - public presets are accessible to anyone
        // Private presets require auth and ownership
        if (!preset.isPublic) {
            // Would need auth check here - for now return 403
            return res.status(403).json({ message: 'This preset is private' });
        }

        // Check if file exists
        if (!fs.existsSync(preset.path)) {
            return res.status(404).json({ message: 'Preset file not found' });
        }

        // Increment download count
        preset.downloads += 1;
        await preset.save();

        // Send file
        res.download(preset.path, preset.filename);
    } catch (error) {
        console.error('Error downloading preset:', error);
        res.status(500).json({ message: 'Server error downloading preset' });
    }
});

// =============================================================================
// PROTECTED ROUTES (require authentication)
// =============================================================================

// @desc    Upload a new preset
// @route   POST /api/presets
// @access  Private
router.post('/', protect, upload.single('presetFile'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ message: 'No preset file uploaded' });
        }

        const { name, description, pluginId, pluginName, category, tags, isPublic } = req.body;

        if (!pluginId || !pluginName) {
            return res.status(400).json({ message: 'Plugin ID and name are required' });
        }

        const preset = await Preset.create({
            owner: req.user._id,
            name: name || req.file.originalname,
            description: description || '',
            pluginId,
            pluginName,
            filename: req.file.filename,
            path: req.file.path,
            size: req.file.size,
            category: category || 'other',
            tags: tags ? tags.split(',').map(t => t.trim()) : [],
            isPublic: isPublic === 'true'
        });

        res.status(201).json(preset);
    } catch (error) {
        console.error('Error uploading preset:', error);
        res.status(500).json({ message: 'Server error during upload' });
    }
});

// @desc    Get user's presets
// @route   GET /api/presets/my
// @access  Private
router.get('/my', protect, async (req, res) => {
    try {
        const presets = await Preset.find({ owner: req.user._id })
            .sort({ updatedAt: -1 });
        res.json(presets);
    } catch (error) {
        console.error('Error fetching user presets:', error);
        res.status(500).json({ message: 'Server error' });
    }
});

// @desc    Update a preset
// @route   PUT /api/presets/:id
// @access  Private (owner only)
router.put('/:id', protect, async (req, res) => {
    try {
        const preset = await Preset.findById(req.params.id);

        if (!preset) {
            return res.status(404).json({ message: 'Preset not found' });
        }

        if (preset.owner.toString() !== req.user._id.toString()) {
            return res.status(403).json({ message: 'Not authorized to update this preset' });
        }

        const { name, description, category, tags, isPublic } = req.body;

        if (name) preset.name = name;
        if (description !== undefined) preset.description = description;
        if (category) preset.category = category;
        if (tags) preset.tags = tags.split(',').map(t => t.trim());
        if (isPublic !== undefined) preset.isPublic = isPublic === 'true' || isPublic === true;

        await preset.save();
        res.json(preset);
    } catch (error) {
        console.error('Error updating preset:', error);
        res.status(500).json({ message: 'Server error' });
    }
});

// @desc    Delete a preset
// @route   DELETE /api/presets/:id
// @access  Private (owner only)
router.delete('/:id', protect, async (req, res) => {
    try {
        const preset = await Preset.findById(req.params.id);

        if (!preset) {
            return res.status(404).json({ message: 'Preset not found' });
        }

        if (preset.owner.toString() !== req.user._id.toString()) {
            return res.status(403).json({ message: 'Not authorized to delete this preset' });
        }

        // Delete file
        if (fs.existsSync(preset.path)) {
            fs.unlinkSync(preset.path);
        }

        await preset.deleteOne();
        res.json({ message: 'Preset deleted' });
    } catch (error) {
        console.error('Error deleting preset:', error);
        res.status(500).json({ message: 'Server error' });
    }
});

// @desc    Like/Unlike a preset
// @route   POST /api/presets/:id/like
// @access  Private
router.post('/:id/like', protect, async (req, res) => {
    try {
        const preset = await Preset.findById(req.params.id);

        if (!preset) {
            return res.status(404).json({ message: 'Preset not found' });
        }

        const userId = req.user._id;
        const alreadyLiked = preset.likedBy.includes(userId);

        if (alreadyLiked) {
            // Unlike
            preset.likedBy = preset.likedBy.filter(id => id.toString() !== userId.toString());
            preset.likes = Math.max(0, preset.likes - 1);
        } else {
            // Like
            preset.likedBy.push(userId);
            preset.likes += 1;
        }

        await preset.save();
        res.json({ likes: preset.likes, liked: !alreadyLiked });
    } catch (error) {
        console.error('Error liking preset:', error);
        res.status(500).json({ message: 'Server error' });
    }
});

module.exports = router;
