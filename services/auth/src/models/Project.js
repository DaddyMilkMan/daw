const mongoose = require('mongoose');

// Version sub-schema for project snapshots
const versionSchema = new mongoose.Schema({
  versionNumber: {
    type: Number,
    required: true
  },
  label: {
    type: String,
    default: ''
  },
  changelog: {
    type: String,
    default: ''
  },
  filename: {
    type: String,
    required: true
  },
  path: {
    type: String,
    required: true
  },
  size: {
    type: Number
  },
  createdAt: {
    type: Date,
    default: Date.now
  }
}, { _id: false });

const projectSchema = new mongoose.Schema({
  owner: {
    type: mongoose.Schema.Types.ObjectId,
    ref: 'User',
    required: true
  },
  name: {
    type: String,
    required: true,
    trim: true
  },
  description: {
    type: String
  },
  filename: {
    type: String,
    required: true
  },
  path: {
    type: String,
    required: true
  },
  size: {
    type: Number
  },
  isPublic: {
    type: Boolean,
    default: false
  },
  // Version history
  versions: [versionSchema],
  createdAt: {
    type: Date,
    default: Date.now
  },
  updatedAt: {
    type: Date,
    default: Date.now
  }
});

const Project = mongoose.model('Project', projectSchema);

module.exports = Project;
